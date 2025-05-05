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
    case CXCursor_ClassDecl: // classes / strcuture
    case CXCursor_StructDecl: {
        DragonEyes::CppClass cls;
        cls.name = toString(clang_getCursorSpelling(c));

        clang_visitChildren(c, [](CXCursor cc, CXCursor parent2, CXClientData clientData2) {
            auto* clsPtr = reinterpret_cast<DragonEyes::CppClass*>(clientData2);
            CXCursorKind k2 = clang_getCursorKind(cc);
            if (k2 == CXCursor_CXXMethod) {
                DragonEyes::Function m;
                m.name = toString(clang_getCursorSpelling(cc));
                m.access = toAccessSpec(cc);
                clsPtr->methods.push_back(std::move(m));
            }
            else if (k2 == CXCursor_FieldDecl) {
                DragonEyes::Variable attr;
                attr.name = toString(clang_getCursorSpelling(cc));
                attr.type = toString(clang_getTypeSpelling(clang_getCursorType(cc)));
                attr.access = toAccessSpec(cc);
                clsPtr->attributes.push_back(std::move(attr));
            }
            return CXChildVisit_Continue;
            }, &cls);
        f->classes.push_back(std::move(cls));
        break;
    }
    case CXCursor_TypedefDecl: {
        TypeAlias ta;
        ta.name = toString(clang_getCursorSpelling(c));
        ta.underlyingType = toString(clang_getTypeSpelling(clang_getTypedefDeclUnderlyingType(c)));

        f->aliases.push_back(std::move(ta));
    }
    case CXCursor_TypeAliasDecl: {
        TypeAlias ta;
        ta.name = toString(clang_getCursorSpelling(c));

        CXType t = clang_getCursorType(c);
        ta.underlyingType = toString(clang_getTypeSpelling(t));
        f->aliases.push_back(std::move(ta));
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