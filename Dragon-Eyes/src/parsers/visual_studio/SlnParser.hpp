#ifndef SLNPARSER_HPP
#define SLNPARSER_HPP

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

#endif // !SLNPARSER_HPP
