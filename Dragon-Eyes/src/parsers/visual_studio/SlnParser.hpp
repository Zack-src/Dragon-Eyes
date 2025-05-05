#pragma once
#include <string>
#include <vector>
#include "../../data_model/DataModel.hpp"

namespace DragonEyes {

    class SlnParser {
    public:
        Solution parseSolution(const std::string& slnPath);

    private:
        std::vector<std::pair<std::string, std::string>>
            extractProjectEntries(const std::string& slnPath);
    };

} // namespace DragonEyes
