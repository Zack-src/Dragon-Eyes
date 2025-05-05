#include "SlnParser.hpp"
#include "VcxprojParser.hpp"
#include <fstream>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;
using namespace DragonEyes;

Solution SlnParser::parseSolution(const std::string& slnPath) {
    Solution sol;
    sol.path = slnPath;

    auto entries = extractProjectEntries(slnPath);
    fs::path slnDir = fs::path(slnPath).parent_path();
    VcxprojParser vcxParser;

    for (auto& [projName, relPath] : entries) {
        fs::path fullProjPath = slnDir / relPath;
        Project proj = vcxParser.parseVcxproj(fullProjPath.string());
        proj.name = projName;
        sol.projects.push_back(std::move(proj));
    }

    return sol;
}

std::vector<std::pair<std::string, std::string>>
SlnParser::extractProjectEntries(const std::string& slnPath) {
    std::vector<std::pair<std::string, std::string>> result;
    std::ifstream in(slnPath);
    std::string line;

    while (std::getline(in, line)) {
        if (line.rfind("Project(", 0) != 0)
            continue;

        size_t p1 = line.find('"');
        if (p1 == std::string::npos) continue;

        size_t p2 = line.find('"', p1 + 1);
        if (p2 == std::string::npos) continue;

        std::string projName = line.substr(p1 + 1, p2 - p1 - 1);

        size_t p3 = line.find('"', p2 + 1);
        if (p3 == std::string::npos) continue;

        size_t p4 = line.find('"', p3 + 1);
        if (p4 == std::string::npos) continue;

        std::string relPath = line.substr(p3 + 1, p4 - p3 - 1);
        result.emplace_back(projName, relPath);
    }

    return result;
}
