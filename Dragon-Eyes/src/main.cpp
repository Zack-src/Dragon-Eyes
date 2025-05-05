
#include <iostream>
#include "parsers/visual_studio/SlnParser.hpp"
#include "data_model/DataModel.hpp"

int main(int argc, char* argv[]) {

    // lecture du projet
    if (argc != 2) {
        std::cerr << "Usage: dragon-eyes <solution.sln>\n";
        return 1;
    }

    std::string slnPath = argv[1];
    DragonEyes::SlnParser parser;
    DragonEyes::Solution sol = parser.parseSolution(slnPath);

    for (auto& proj : sol.projects) {
        std::cout << "Projet : " << proj.name << "\n";
        for (auto& file : proj.files) {
            std::cout << "  - " << file.path << "\n";
        }
    }

    // generation du graph

    // detection des bug

    // detection des améliorations

    return 0;
}
