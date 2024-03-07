#include <string>
#include <vector>

#ifndef TOKENIZER
#define TOKENIZER

namespace Tokenizer
{
    enum class TokenType
    {
        COMMAND,
        ARGUMENT,
        PIPE,
        REDIRECTION
    };

    struct Token
    {
        TokenType type;
        std::string value;
    };

    std::vector<Token> tokenize(const std::string &input);
}

#endif