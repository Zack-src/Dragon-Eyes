#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

#include "parsers/visual_studio/SlnParser.hpp"
#include "data_model/DataModel.hpp"
#include "parsers/code/ASTParser.hpp"
#include "parsers/visual_studio/VcxprojParser.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: dragon-eyes <solution.sln>\n";
        return 1;
    }

    std::string inputPath = argv[1];
    DragonEyes::Solution sol;

    std::filesystem::path p(inputPath);
    auto ext = p.extension().string();
    if (ext == ".sln") {
        DragonEyes::SlnParser slnParser;
        sol = slnParser.parseSolution(inputPath);
        sol.path = inputPath;
    } else if (ext == ".vcxproj") {
        DragonEyes::VcxprojParser vcxParser;
        DragonEyes::Project proj = vcxParser.parseVcxproj(inputPath);
        proj.name = p.stem().string();
        sol.path = inputPath;
        sol.projects.push_back(std::move(proj));
    } else {
        std::cerr << "Erreur: format non supporte ("
            << ext << "). Utilisez .sln ou .vcxproj.\n";
        return 1;
    }

    std::vector<std::string> clangArgs = {"-std=c++20"};
    DragonEyes::ASTParser astParser(clangArgs);

    for (auto& proj : sol.projects) {
        std::cout << "Projet : " << proj.name << "\n";
        for (auto& file : proj.files) {
            std::cout << "  - Fichier : " << file.path << "\n";

            // Analyse AST
            astParser.parseFile(file);

            // Métadonnées
            std::cout << "    Exists: " << (file.exists ? "yes" : "no") << "\n";
            if (file.exists) {
                if (file.size) std::cout << "    Size: " << *file.size << " bytes\n";
                if (file.lastWrite) {
                    auto tp = *file.lastWrite;
                    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(
                        tp.time_since_epoch()).count();
                    std::cout << "    LastWrite (epoch): " << epoch << "\n";
                }
            }

            // Variables globales
            if (!file.globals.empty()) {
                std::cout << "    Variables globales :\n";
                for (auto& var : file.globals) {
                    std::cout << "      - " << var.type << " " << var.name << "\n";
                }
            }

            // Fonctions libres
            if (!file.functions.empty()) {
                std::cout << "    Fonctions libres :\n";
                for (auto& fn : file.functions) {
                    std::cout << "      - " << fn.name << "(";
                    for (size_t i = 0; i < fn.parameters.size(); ++i) {
                        auto& p = fn.parameters[i];
                        std::cout << p.type << " " << p.name
                            << (i+1 < fn.parameters.size() ? ", " : "");
                    }
                    std::cout << ")\n";
                }
            }

            // Alias de types
            if (!file.aliases.empty()) {
                std::cout << "    Alias de types :\n";
                for (auto& a : file.aliases) {
                    std::cout << "      - " << a.name << " = " << a.underlyingType << "\n";
                }
            }

            // Classes / Structs
            if (!file.classes.empty()) {
                std::cout << "    Classes/Structs :\n";
                for (auto& cls : file.classes) {
                    std::cout << "      + Classe : " << cls.name << "\n";

                    // Heritage
                    if (!cls.baseClasses.empty()) {
                        std::cout << "        Herite de : ";
                        for (auto& b : cls.baseClasses) std::cout << b << " ";
                        std::cout << "\n";
                    }

                    // Attributs par niveau d'accès
                    if (!cls.publicAttributes.empty()) {
                        std::cout << "        Attributs publics :\n";
                        for (auto& attr : cls.publicAttributes) {
                            std::cout << "          - " << attr.type
                                << " " << attr.name << "\n";
                        }
                    }
                    if (!cls.protectedAttributes.empty()) {
                        std::cout << "        Attributs protected :\n";
                        for (auto& attr : cls.protectedAttributes) {
                            std::cout << "          - " << attr.type
                                << " " << attr.name << "\n";
                        }
                    }
                    if (!cls.privateAttributes.empty()) {
                        std::cout << "        Attributs private :\n";
                        for (auto& attr : cls.privateAttributes) {
                            std::cout << "          - " << attr.type
                                << " " << attr.name << "\n";
                        }
                    }

                    // Méthodes par niveau d'accès
                    if (!cls.publicMethods.empty()) {
                        std::cout << "        Methodes public :\n";
                        for (auto& m : cls.publicMethods) {
                            std::cout << "          - " << m.name << "(";
                            for (size_t i = 0; i < m.parameters.size(); ++i) {
                                auto& p = m.parameters[i];
                                std::cout << p.type << " " << p.name
                                    << (i+1 < m.parameters.size() ? ", " : "");
                            }
                            std::cout << ")\n";

                            if (!m.localVariables.empty()) {
                                std::cout << "            Variables locales :\n";
                                for (auto& lv : m.localVariables) {
                                    std::cout << "              * " << lv.type
                                        << " " << lv.name << "\n";
                                }
                            }
                            if (!m.calledFunctions.empty()) {
                                std::cout << "            Appels de fonctions :\n";
                                for (auto& cf : m.calledFunctions) {
                                    std::cout << "              * " << cf << "\n";
                                }
                            }
                        }
                    }
                    if (!cls.protectedMethods.empty()) {
                        std::cout << "        Methodes protected :\n";
                        for (auto& m : cls.protectedMethods) {
                            std::cout << "          - " << m.name << "()\n";
                        }
                    }
                    if (!cls.privateMethods.empty()) {
                        std::cout << "        Methodes private :\n";
                        for (auto& m : cls.privateMethods) {
                            std::cout << "          - " << m.name << "()\n";
                        }
                    }
                }
            }

            std::cout << std::string(60, '-') << "\n";
        }

        // Fichiers manquants
        if (!proj.missingFiles.empty()) {
            std::cerr << "  Fichiers manquants :\n";
            for (auto& mf : proj.missingFiles) {
                std::cerr << "    * " << mf << "\n";
            }
        }
    }

    // generation du graph

    // detection des bug

    // detection des ameliorations

    return 0;
}
