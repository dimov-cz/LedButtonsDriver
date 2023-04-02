#ifndef LEDANIMATION_DEFPARSER_H
#define LEDANIMATION_DEFPARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

class LedAnimationDefinitionParser {
    public:
        std::vector<std::vector<int>> interpretDefinitionString(const std::string& input, const std::string& defaultString);
    private:
        std::vector<std::string> parseStringByGroups(const std::string& input);
        std::vector<std::string> parseStringByDelimiter(const std::string& str, char delimiter);
        int ignorantStoi(std::string input, int defaultValue);
};

#endif // ifndef LEDANIMATION_DEFPARSER_H