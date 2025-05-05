#include "VcxprojParser.hpp"
#include "tinyxml2.h"
#include <filesystem>

namespace fs = std::filesystem;
using namespace tinyxml2;
using namespace DragonEyes;

Project VcxprojParser::parseVcxproj(const std::string& vcxprojPath) {
    Project project;
    project.path = vcxprojPath;
    project.name = fs::path(vcxprojPath).stem().string();

    XMLDocument doc;
    if (doc.LoadFile(vcxprojPath.c_str()) != XML_SUCCESS) {
        return project;
    }

    XMLElement* root = doc.RootElement();
    for (XMLElement* ig = root->FirstChildElement("ItemGroup"); ig; ig = ig->NextSiblingElement("ItemGroup")) {
        for (XMLElement* cl = ig->FirstChildElement("ClCompile"); cl; cl = cl->NextSiblingElement("ClCompile")) {
            if (auto* inc = cl->Attribute("Include")) {
                SourceFile f;
                fs::path p = fs::path(vcxprojPath).parent_path() / inc;
                f.path = fs::weakly_canonical(p).string();
                project.files.push_back(std::move(f));
            }
        }
        for (XMLElement* incE = ig->FirstChildElement("ClInclude"); incE; incE = incE->NextSiblingElement("ClInclude")) {
            if (auto* inc = incE->Attribute("Include")) {
                SourceFile f;
                fs::path p = fs::path(vcxprojPath).parent_path() / inc;
                f.path = fs::weakly_canonical(p).string();
                project.files.push_back(std::move(f));
            }
        }
    }
    return project;
}
