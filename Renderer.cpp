#include "Renderer.h"
#include <iostream>
#include <algorithm>

void Renderer::render(const std::vector<Token>& tokens)
{
    bool ignoreContent = false;

    for (const auto& token : tokens)
    {
        if (token.type == TokenType::TagOpen)
        {
            std::string tag = token.value;
            std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

            if (tag == "script" || tag == "style" || tag == "svg") ignoreContent = true;
            else if (tag == "h1" || tag == "h2" || tag == "h3") std::cout << "\n\n=== ";
            else if (tag == "p" || tag == "br") std::cout << '\n';
            else if (tag == "li") std::cout << "\n • ";
            else if (tag == "b" || tag == "strong") std::cout << "\033[1m";
        }
        else if (token.type == TokenType::TagClose)
        {
            std::string tag = token.value;
            std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

            if (tag == "script" || tag == "style" || tag == "svg") ignoreContent = false;
            else if (tag == "h1" || tag == "h2" || tag == "h3") std::cout << " ===\n";
            else if (tag == "b" || tag == "strong") std::cout << "\033[0m";
        }
        else if (token.type == TokenType::Text && !ignoreContent)
        {
            std::cout << token.value;
        }
    }
    std::cout << "\033[0m\n";
}
