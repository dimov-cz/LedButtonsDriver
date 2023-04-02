#include "DefinitionParser.h"



std::vector<std::string> LedAnimationDefinitionParser::parseStringByGroups(const std::string& input) {
    std::vector<std::string> output;
    std::string current;

    bool inList = false;
    for (char c : input) {
        if (c == '[') {
            inList = true;
        } else if (c == ']') {
            inList = false;
        } else if (c == ',' && !inList) {
            output.push_back(current);
            current = "";
        } else {
            current += c;
        }
    }

    // Handle last element
    if (!current.empty()) {
        output.push_back(current);
    }

    return output;
}

std::vector<std::string> LedAnimationDefinitionParser::parseStringByDelimiter(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int LedAnimationDefinitionParser::ignorantStoi(std::string input, int defaultValue)
{
    try {
        return std::stoi(input);
    } catch (const std::invalid_argument& e) {
        //std::cerr << "Invalid argument: " << e.what() << std::endl;
    } catch (const std::out_of_range& e) {
        //std::cerr << "Out of range: " << e.what() << std::endl;
    }
    return defaultValue;
}

std::vector<std::vector<int>> LedAnimationDefinitionParser::interpretDefinitionString(const std::string& input, const std::string& defaultString) 
{
    std::vector<std::vector<int>> out;
    std::vector<int> currentList;
    int i;

    std::vector<int> defaults;
    for (const auto& item : this->parseStringByDelimiter(defaultString, ',')) {
        defaults.push_back(ignorantStoi(item, 0));
    }

    for (const auto& stepString : parseStringByGroups(input)) 
    {
        std::vector<std::string> inputPart = parseStringByDelimiter(stepString, ',');
        for(i=0; i<defaults.size();i++){
            if (i<inputPart.size() && !inputPart[i].empty()){
                int value = ignorantStoi(inputPart[i], defaults[i]);
                currentList.push_back(value);
                defaults[i] = value;
                
            }else{
                currentList.push_back(defaults[i]);
            }
        }
        out.push_back(currentList);
        currentList.clear();
    }
    return out;
}
