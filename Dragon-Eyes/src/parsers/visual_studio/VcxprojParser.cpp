#include "VcxprojParser.hpp"
#include "../../data_model/DataModel.hpp"

#include "tinyxml2.h"
#include <filesystem>
#include <iostream>


namespace fs = std::filesystem;
using namespace tinyxml2;
using namespace DragonEyes;

Project VcxprojParser::parseVcxproj(const std::string& vcxprojPath) {
    Project project;
    project.path = vcxprojPath;
    project.name = fs::path(vcxprojPath).stem().string();

    XMLDocument doc;
    if (doc.LoadFile(vcxprojPath.c_str()) != XML_SUCCESS) {
        std::cerr << "Erreur: impossible de charger " << vcxprojPath << "\n";
        return project;
    }

    XMLElement* root = doc.RootElement();
    for (XMLElement* ig = root->FirstChildElement("ItemGroup"); ig; ig = ig->NextSiblingElement("ItemGroup")) {

        for (const char* tag : { "ClCompile", "ClInclude" }) {
            for (XMLElement* el = ig->FirstChildElement(tag); el; el = el->NextSiblingElement(tag)) {
                if (auto* inc = el->Attribute("Include")) {
                    fs::path p = fs::path(vcxprojPath).parent_path() / inc;
                    SourceFile f;
                    f.path = fs::weakly_canonical(p).string();

                    try {
                        f.exists = fs::exists(p);
                        if (f.exists) {
                            f.size = fs::file_size(p);
                            f.lastWrite = fs::last_write_time(p);
                        }
                        else {
                            project.missingFiles.push_back(f.path);
                        }
                    }
                    catch (const fs::filesystem_error& e) {
                        std::cerr << "FS error sur " << p << " : " << e.what() << "\n";
                        f.exists = false;
                        project.missingFiles.push_back(f.path);
                    }

                    project.files.push_back(std::move(f));
                }
            }
        }
    }

    return project;
}
