#pragma once
#include <string>
#include <vector>
#include <string_view>

enum class TokenType { Text, TagOpen, TagClose };

struct Token
{
    TokenType type;
    std::string value;
    std::string attr;
};

class HtmlLexer
{
public:
    static std::vector<Token> tokenize(std::string_view html);
};