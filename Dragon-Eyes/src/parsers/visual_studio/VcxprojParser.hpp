
#ifndef VCXPROJPARSER_HPP
#define VCXPROJPARSER_HPP


#include <string>
#include "../../data_model/DataModel.hpp"

namespace DragonEyes {

    class VcxprojParser {
    public:
        Project parseVcxproj(const std::string& vcxprojPath);
    };

} // namespace DragonEyes

#endif // !VCXPROJPARSER_HPP