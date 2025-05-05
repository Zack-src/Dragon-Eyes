
#include <iostream>
#include "parsers/visual_studio/SlnParser.hpp"
#include "data_model/DataModel.hpp"
#include "parsers/code/ASTParser.hpp"

int main(int argc, char* argv[]) {

    // lecture du projet
    if (argc != 2) {
        std::cerr << "Usage: dragon-eyes <solution.sln>\n";
        return 1;
    }

    std::string slnPath = argv[1];
    DragonEyes::SlnParser parser;
    DragonEyes::Solution sol = parser.parseSolution(slnPath);

    std::vector<std::string> clangArgs = {
        "-std=c++20",
    };
    DragonEyes::ASTParser astParser(clangArgs);

    for (auto& proj : sol.projects) {
        std::cout << "Projet : " << proj.name << "\n";
        for (auto& file : proj.files) {
            std::cout << "  - " << file.path << "\n";
            astParser.parseFile(file);

            // 1) Variables globales
            if (!file.globals.empty()) {
                std::cout << "      Variables globales :\n";
                for (auto& var : file.globals) {
                    std::cout << "        - " << var.type
                        << " " << var.name << "\n";
                }
            }

            // 2) Fonctions libres
            if (!file.functions.empty()) {
                std::cout << "      Fonctions libres :\n";
                for (auto& fn : file.functions) {
                    std::cout << "        - " << fn.name << "(";
                    for (size_t i = 0; i < fn.parameters.size(); ++i) {
                        auto& p = fn.parameters[i];
                        std::cout << p.type << " " << p.name
                            << (i + 1 < fn.parameters.size() ? ", " : "");
                    }
                    std::cout << ")\n";
                }
            }

            // 3) Classes / Structs
            if (!file.classes.empty()) {
                std::cout << "      Classes/Structs :\n";
                for (auto& cls : file.classes) {
                    std::cout << "        • Classe « " << cls.name << " »\n";

                    // 4.a) Bases
                    if (!cls.baseClasses.empty()) {
                        std::cout << "          Hérite de :\n";
                        for (auto& b : cls.baseClasses) {
                            std::cout << "            - " << b << "\n";
                        }
                    }

                    // 4.b) Attributs
                    auto printAttrs = [&](const std::vector<DragonEyes::Variable>& attrs, const char* title) {
                        if (!attrs.empty()) {
                            std::cout << "          " << title << " :\n";
                            for (auto& attr : attrs) {
                                std::cout << "            - " << attr.type
                                    << " " << attr.name << "\n";
                            }
                        }
                        };
                    printAttrs(cls.publicAttributes, "Attributs publics");
                    printAttrs(cls.protectedAttributes, "Attributs protégés");
                    printAttrs(cls.privateAttributes, "Attributs privés");

                    // 4.c) Méthodes
                    auto printMeths = [&](const std::vector<DragonEyes::Function>& meths, const char* title) {
                        if (!meths.empty()) {
                            std::cout << "          " << title << " :\n";
                            for (auto& m : meths) {
                                std::cout << "            - " << m.name << "(";
                                for (size_t i = 0; i < m.parameters.size(); ++i) {
                                    auto& p = m.parameters[i];
                                    std::cout << p.type << " " << p.name
                                        << (i + 1 < m.parameters.size() ? ", " : "");
                                }
                                std::cout << ")\n";
                            }
                        }
                        };
                    printMeths(cls.publicMethods, "Méthodes publiques");
                    printMeths(cls.protectedMethods, "Méthodes protégées");
                    printMeths(cls.privateMethods, "Méthodes privées");
                }
            }

            // 4) Type aliases
            if (!file.aliases.empty()) {
                std::cout << "      Alias de types :\n";
                for (auto& a : file.aliases) {
                    std::cout << "        - " << a.name << " = " << a.underlyingType << "\n";
                }
            }

            std::cout << std::string(60, '-') << "\n";
        }

        if (!proj.missingFiles.empty()) {
            std::cerr << "  Fichiers manquants :\n";
            for (auto& m : proj.missingFiles) {
                std::cerr << "    * " << m << "\n";
            }
        }
    }

    // generation du graph

    // detection des bug

    // detection des améliorations

    return 0;
}
