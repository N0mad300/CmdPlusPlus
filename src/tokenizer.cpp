#include "Tokenizer.h"
#include <sstream>
#include <iomanip>

namespace Tokenizer
{
    std::vector<Token> tokenize(const std::string &input)
    {
        std::vector<Token> tokens;
        std::istringstream stream(input);
        std::string token;

        bool inQuotes = false;

        while (stream >> std::ws)
        {
            if (stream.peek() == '"')
            {
                stream.ignore();
                std::getline(stream, token, '"');
                inQuotes = true;
            }
            else
            {
                stream >> token;
                inQuotes = false;
            }

            if (token == "|")
            {
                tokens.push_back({TokenType::PIPE, token});
            }
            else if (token == ">")
            {
                tokens.push_back({TokenType::REDIRECTION, token});
            }
            else
            {
                tokens.push_back({TokenType::ARGUMENT, token});
            }

            if (inQuotes)
            {
                stream.ignore(); // Skip the closing quote
            }
        }

        return tokens;
    }
}