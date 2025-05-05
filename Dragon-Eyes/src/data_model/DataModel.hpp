#pragma once
#include <string>
#include <vector>

namespace DragonEyes {

    enum class AccessSpecifier { Public, Protected, Private };

    struct Variable {
        std::string name;
        std::string type;
        AccessSpecifier access = AccessSpecifier::Private;
    };

    struct Function {
        std::string name;
        std::vector<Variable> parameters;
        std::vector<std::string> calledFunctions;
        AccessSpecifier access = AccessSpecifier::Private;
    };

    struct CppClass {
        std::string name;
        std::vector<Variable> attributes;
        std::vector<Function> methods;
    };

    struct SourceFile {
        std::string path;
        std::vector<Variable> globals;
        std::vector<CppClass> classes;
        std::vector<Function> functions;
    };

    struct Project {
        std::string name;
        std::string path;
        std::vector<SourceFile> files;
    };

    struct Solution {
        std::string path;
        std::vector<Project> projects;
    };

} // namespace DragonEyes
