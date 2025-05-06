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
    auto* f = reinterpret_cast<SourceFile*>(clientData);

    // 1) Ne traiter que le fichier principal
    CXSourceLocation loc = clang_getCursorLocation(c);
    if (!clang_Location_isFromMainFile(loc))
        return CXChildVisit_Continue;

    CXCursorKind kind = clang_getCursorKind(c);
    switch (kind) {
    //--- Namespace: descendre pour trouver inline globals ---
    case CXCursor_Namespace: {
        clang_visitChildren(c, visitor, clientData);
        break;
    }

    //--- Variables globales ou inline namespace vars ---
    case CXCursor_VarDecl: {
        CXCursor semParent = clang_getCursorSemanticParent(c);
        CXCursorKind pkind = clang_getCursorKind(semParent);
        if (pkind == CXCursor_TranslationUnit || pkind == CXCursor_Namespace) {
            Variable var;
            var.name   = toString(clang_getCursorSpelling(c));
            var.type   = toString(clang_getTypeSpelling(clang_getCursorType(c)));
            var.access = AccessSpecifier::Public;
            f->globals.push_back(std::move(var));
        }
        break;
    }

    //--- Fonctions libres ---
    case CXCursor_FunctionDecl: {
        CXCursor semParent = clang_getCursorSemanticParent(c);
        if (clang_getCursorKind(semParent) == CXCursor_TranslationUnit) {
            Function fn;
            fn.name   = toString(clang_getCursorSpelling(c));
            fn.access = AccessSpecifier::Public;
            // paramètres
            int nargs = clang_Cursor_getNumArguments(c);
            for (int i = 0; i < nargs; ++i) {
                CXCursor arg = clang_Cursor_getArgument(c, i);
                Variable p;
                p.name   = toString(clang_getCursorSpelling(arg));
                p.type   = toString(clang_getTypeSpelling(clang_getCursorType(arg)));
                p.access = AccessSpecifier::Public;
                fn.parameters.push_back(std::move(p));
            }
            f->functions.push_back(std::move(fn));
        }
        break;
    }

    //--- Typedef et using ---
    case CXCursor_TypedefDecl: {
        TypeAlias ta;
        ta.name           = toString(clang_getCursorSpelling(c));
        ta.underlyingType = toString(
            clang_getTypeSpelling(
                clang_getTypedefDeclUnderlyingType(c)
            )
        );
        if (ta.underlyingType != ta.name)
            f->aliases.push_back(std::move(ta));
        break;
    }
    case CXCursor_TypeAliasDecl: {
        TypeAlias ta;
        ta.name           = toString(clang_getCursorSpelling(c));
        CXType t          = clang_getCursorType(c);
        ta.underlyingType = toString(clang_getTypeSpelling(t));
        if (ta.underlyingType != ta.name)
            f->aliases.push_back(std::move(ta));
        break;
    }

    //--- Enumérations ---
    case CXCursor_EnumDecl: {
        // parcourir les constantes
        clang_visitChildren(c, [](CXCursor cc, CXCursor, CXClientData) {
            if (clang_getCursorKind(cc) == CXCursor_EnumConstantDecl) {
                std::string label = toString(clang_getCursorSpelling(cc));
                long long val = clang_getEnumConstantDeclValue(cc);
                std::string value = std::to_string(val);
                // Pour l'instant on affiche ; vous pouvez stocker ailleurs
                std::cout << "    Enum constant: " << label << " = " << value << "\n";
            }
            return CXChildVisit_Continue;
            }, nullptr);
        break;
    }

    //--- Définition d'une classe/struct ---
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl: {
        CppClass cls;
        cls.name = toString(clang_getCursorSpelling(c));
        // nappes de base
        clang_visitChildren(c, [](CXCursor cc, CXCursor, CXClientData clientData) {
            auto* clsPtr = reinterpret_cast<CppClass*>(clientData);
            CXCursorKind k2 = clang_getCursorKind(cc);
            if (k2 == CXCursor_CXXBaseSpecifier) {
                std::string base = toString(
                    clang_getTypeSpelling(clang_getCursorType(cc))
                );
                clsPtr->baseClasses.push_back(std::move(base));
            }
            return CXChildVisit_Continue;
            }, &cls);
        // champs et méthodes internes
        clang_visitChildren(c, [](CXCursor cc, CXCursor, CXClientData clientData) {
            auto* clsPtr = reinterpret_cast<CppClass*>(clientData);
            CXCursorKind k2 = clang_getCursorKind(cc);
            AccessSpecifier acc = toAccessSpec(cc);
            if (k2 == CXCursor_FieldDecl) {
                Variable attr;
                attr.name   = toString(clang_getCursorSpelling(cc));
                attr.type   = toString(
                    clang_getTypeSpelling(clang_getCursorType(cc))
                );
                attr.access = acc;
                if (acc == AccessSpecifier::Public)
                    clsPtr->publicAttributes.push_back(std::move(attr));
                else if (acc == AccessSpecifier::Protected)
                    clsPtr->protectedAttributes.push_back(std::move(attr));
                else
                    clsPtr->privateAttributes.push_back(std::move(attr));
            }
            else if (k2 == CXCursor_CXXMethod) {
                Function m;
                m.name   = toString(clang_getCursorSpelling(cc));
                m.access = acc;
                int nargs = clang_Cursor_getNumArguments(cc);
                for (int i = 0; i < nargs; ++i) {
                    CXCursor arg = clang_Cursor_getArgument(cc, i);
                    Variable p;
                    p.name   = toString(clang_getCursorSpelling(arg));
                    p.type   = toString(
                        clang_getTypeSpelling(clang_getCursorType(arg))
                    );
                    p.access = AccessSpecifier::Public;
                    m.parameters.push_back(std::move(p));
                }
                if (acc == AccessSpecifier::Public)
                    clsPtr->publicMethods.push_back(std::move(m));
                else if (acc == AccessSpecifier::Protected)
                    clsPtr->protectedMethods.push_back(std::move(m));
                else
                    clsPtr->privateMethods.push_back(std::move(m));
            }
            return CXChildVisit_Continue;
            }, &cls);
        f->classes.push_back(std::move(cls));
        break;
    }

                            //--- Méthodes définies hors de la classe (.cpp) ---
    case CXCursor_CXXMethod: {
        if (!clang_isCursorDefinition(c))
            break;
        CXCursor semParent = clang_getCursorSemanticParent(c);
        if (clang_getCursorKind(semParent) != CXCursor_ClassDecl &&
            clang_getCursorKind(semParent) != CXCursor_StructDecl)
            break;
        std::string clsName = toString(
            clang_getCursorSpelling(semParent)
        );
        auto it = std::find_if(
            f->classes.begin(), f->classes.end(),
            [&](auto& cls){ return cls.name == clsName; }
        );
        if (it == f->classes.end()) break;
        CppClass& cls = *it;

        Function m;
        m.name   = toString(clang_getCursorSpelling(c));
        m.access = toAccessSpec(c);
        int nargs = clang_Cursor_getNumArguments(c);
        for (int i = 0; i < nargs; ++i) {
            CXCursor arg = clang_Cursor_getArgument(c, i);
            Variable p;
            p.name   = toString(clang_getCursorSpelling(arg));
            p.type   = toString(
                clang_getTypeSpelling(clang_getCursorType(arg))
            );
            p.access = AccessSpecifier::Public;
            m.parameters.push_back(std::move(p));
        }
        // corps de la méthode : locales & appels
        clang_visitChildren(c, [](CXCursor cc, CXCursor, CXClientData clientData) {
            auto* mPtr = reinterpret_cast<Function*>(clientData);
            CXCursorKind k2 = clang_getCursorKind(cc);
            if (k2 == CXCursor_VarDecl) {
                Variable v;
                v.name   = toString(clang_getCursorSpelling(cc));
                v.type   = toString(
                    clang_getTypeSpelling(clang_getCursorType(cc))
                );
                v.access = AccessSpecifier::Private;
                mPtr->localVariables.push_back(std::move(v));
            } else if (k2 == CXCursor_CallExpr) {
                std::string called = toString(
                    clang_getCursorSpelling(cc)
                );
                if (!called.empty())
                    mPtr->calledFunctions.push_back(called);
            }
            return CXChildVisit_Recurse;
            }, &m);

        // injection
        if (m.access == AccessSpecifier::Public)
            cls.publicMethods.push_back(std::move(m));
        else if (m.access == AccessSpecifier::Protected)
            cls.protectedMethods.push_back(std::move(m));
        else
            cls.privateMethods.push_back(std::move(m));
        break;
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