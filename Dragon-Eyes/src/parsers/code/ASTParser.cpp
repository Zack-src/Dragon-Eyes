#include "ASTParser.hpp"

#include <filesystem>
#include <iostream>

DragonEyes::ASTParser::ASTParser(const std::vector<std::string>& args) {
    index_ = clang_createIndex(0, 0);
    for (auto& a : args)
        clangArgs_.push_back(a.c_str());
}

DragonEyes::ASTParser::~ASTParser() {
    clang_disposeIndex(index_);
}

void DragonEyes::ASTParser::parseFile(SourceFile& f) {
    if (!f.exists) return;

    CXTranslationUnit tu = clang_parseTranslationUnit(
        index_,
        f.path.c_str(),
        clangArgs_.data(), 
        clangArgs_.size(),
        nullptr, 0,
        CXTranslationUnit_None
    );
    if (!tu) {
        std::cerr << "Échec de l’analyse AST de " << f.path << "\n";
        return;
    }

    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(rootCursor, visitor, &f);

    clang_disposeTranslationUnit(tu);
}

CXChildVisitResult DragonEyes::ASTParser::visitor(CXCursor c, CXCursor parent, CXClientData clientData) {

    CXSourceLocation loc = clang_getCursorLocation(c);
    if (!clang_Location_isFromMainFile(loc))
        return CXChildVisit_Continue;

    auto* f = reinterpret_cast<DragonEyes::SourceFile*>(clientData);
    CXCursorKind kind = clang_getCursorKind(c);

    switch (kind) {
    case CXCursor_VarDecl: { // variable global
        DragonEyes::Variable var;
        var.name = toString(clang_getCursorSpelling(c));
        var.type = toString(clang_getTypeSpelling(clang_getCursorType(c)));
        var.access = AccessSpecifier::Public;
        f->globals.push_back(std::move(var));
        break;
    }
    case CXCursor_FunctionDecl: { // free function
        DragonEyes::Function fn;
        fn.name = toString(clang_getCursorSpelling(c));
        fn.access = AccessSpecifier::Public;

        int numArgs = clang_Cursor_getNumArguments(c);
        for (int i = 0; i < numArgs; ++i) {
            CXCursor arg = clang_Cursor_getArgument(c, i);
            DragonEyes::Variable v;
            v.name = toString(clang_getCursorSpelling(arg));
            v.type = toString(clang_getTypeSpelling(clang_getCursorType(arg)));
            v.access = AccessSpecifier::Public;
            fn.parameters.push_back(std::move(v));
        }
        f->functions.push_back(std::move(fn));
        break;
    }
    case CXCursor_StructDecl:
    case CXCursor_ClassDecl: {

        DragonEyes::CppClass cls;
        cls.name = toString(clang_getCursorSpelling(c));

        clang_visitChildren(
            c,
            [](CXCursor cc, CXCursor parent2, CXClientData clientData) {
                auto* clsPtr = reinterpret_cast<DragonEyes::CppClass*>(clientData);
                CXCursorKind kind = clang_getCursorKind(cc);
                AccessSpecifier acc = toAccessSpec(cc);

                // 2.a) Spécificateurs de base (parents)
                if (kind == CXCursor_CXXBaseSpecifier) {
                    // type de la base (nom de la classe parente)
                    std::string baseName =
                        toString(clang_getTypeSpelling(clang_getCursorType(cc)));
                    clsPtr->baseClasses.push_back(std::move(baseName));
                }
                // 2.b) Attributs (FieldDecl)
                else if (kind == CXCursor_FieldDecl) {
                    DragonEyes::Variable attr;
                    attr.name = toString(clang_getCursorSpelling(cc));
                    attr.type = toString(
                        clang_getTypeSpelling(clang_getCursorType(cc)));
                    attr.access = acc;

                    // ajout suivant l’accès
                    if (acc == AccessSpecifier::Public)
                        clsPtr->publicAttributes.push_back(std::move(attr));
                    else if (acc == AccessSpecifier::Protected)
                        clsPtr->protectedAttributes.push_back(std::move(attr));
                    else
                        clsPtr->privateAttributes.push_back(std::move(attr));
                }
                // 2.c) Méthodes (CXXMethod)
                else if (kind == CXCursor_CXXMethod) {
                    DragonEyes::Function m;
                    m.name = toString(clang_getCursorSpelling(cc));
                    m.access = acc;
                    // récupération des paramètres si nécessaire :
                    int nargs = clang_Cursor_getNumArguments(cc);
                    for (int i = 0; i < nargs; ++i) {
                        CXCursor arg = clang_Cursor_getArgument(cc, i);
                        DragonEyes::Variable p;
                        p.name = toString(clang_getCursorSpelling(arg));
                        p.type = toString(
                            clang_getTypeSpelling(clang_getCursorType(arg)));
                        p.access = AccessSpecifier::Public;
                        m.parameters.push_back(std::move(p));
                    }

                    // ajout suivant l’accès
                    if (acc == AccessSpecifier::Public)
                        clsPtr->publicMethods.push_back(std::move(m));
                    else if (acc == AccessSpecifier::Protected)
                        clsPtr->protectedMethods.push_back(std::move(m));
                    else
                        clsPtr->privateMethods.push_back(std::move(m));
                }

                return CXChildVisit_Recurse;
            },
            &cls
        );

        // 3) On enregistre la classe complète dans le fichier
        f->classes.push_back(std::move(cls));
        break;
    }

    case CXCursor_TypedefDecl: {
        TypeAlias ta;
        ta.name = toString(clang_getCursorSpelling(c));
        ta.underlyingType = toString(clang_getTypeSpelling(clang_getTypedefDeclUnderlyingType(c)));

        if (ta.underlyingType != ta.name) {
            f->aliases.push_back(std::move(ta));
        }
    }
    case CXCursor_TypeAliasDecl: {
        TypeAlias ta;
        ta.name = toString(clang_getCursorSpelling(c));

        CXType t = clang_getCursorType(c);
        ta.underlyingType = toString(clang_getTypeSpelling(t));

        if (ta.underlyingType != ta.name) {
            f->aliases.push_back(std::move(ta));
        }
    }
    default:
        break;
    }

    return CXChildVisit_Recurse;
}

std::string DragonEyes::ASTParser::toString(CXString s) {
    std::string str = clang_getCString(s);
    clang_disposeString(s);
    return str;
}

DragonEyes::AccessSpecifier DragonEyes::ASTParser::toAccessSpec(CXCursor c) {
    switch (clang_getCXXAccessSpecifier(c)) {
    case CX_CXXPublic:    return DragonEyes::AccessSpecifier::Public;
    case CX_CXXProtected: return DragonEyes::AccessSpecifier::Protected;
    case CX_CXXPrivate:   return DragonEyes::AccessSpecifier::Private;
    default:              return DragonEyes::AccessSpecifier::Private;
    }
}