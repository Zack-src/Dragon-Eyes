#include "SlnParser.hpp"
#include "VcxprojParser.hpp"
#include <fstream>
#include <regex>
#include <filesystem>
#include <iostream>

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

        std::cout << "parsed : " << projName << " " << relPath << "\t" << fullProjPath.string() << std::endl;

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

        std::vector<std::string> elems;
        size_t pos = 0;
        while (true) {
            size_t start = line.find('"', pos);
            if (start == std::string::npos) break;
            
            size_t end = line.find('"', start + 1);
            if (end == std::string::npos) break;
            
            elems.emplace_back(line.substr(start + 1, end - start - 1));
            pos = end + 1;
        }

        if (elems.size() >= 3) {
            const std::string& projName = elems[1];
            const std::string& relPath = elems[2];
            result.emplace_back(projName, relPath);
        }
    }

    return result;
}

