
#ifndef DATAMODEL_HPP
#define DATAMODEL_HPP

#include <string>
#include <vector>
#include <optional>
#include <chrono>

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
        std::vector<Variable> localVariables;
        std::vector<std::string> calledFunctions;
        AccessSpecifier access = AccessSpecifier::Private;
    };

    struct CppClass {
        std::string name;
        std::vector<std::string> baseClasses;

        std::vector<Variable> publicAttributes;
        std::vector<Variable> privateAttributes;
        std::vector<Variable> protectedAttributes;

        std::vector<Function> publicMethods;
        std::vector<Function> privateMethods;
        std::vector<Function> protectedMethods;
    };

    struct TypeAlias
    {
        std::string name;
        std::string underlyingType;
    };

    struct SourceFile {
        bool exists = false;
        std::optional<uintmax_t> size = {};
        std::optional<std::chrono::file_clock::time_point> lastWrite = {};

        std::string path;
        std::vector<Variable> globals;
        std::vector<CppClass> classes;
        std::vector<Function> functions;

        std::vector<TypeAlias> aliases;
    };

    struct Project {
        std::string name;
        std::string path;
        std::vector<SourceFile> files;
        std::vector<std::string> missingFiles;
    };

    struct Solution {
        std::string path;
        std::vector<Project> projects;
    };

} // namespace DragonEyes

#endif // !DATAMODEL_HPP
