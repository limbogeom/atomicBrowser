#include "HtmlLexer.h"
#include <cctype>
#include <algorithm>

std::vector<Token> HtmlLexer::tokenize(std::string_view html)
{
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < html.length())
    {
        if (html[i] == '<')
        {
            size_t end = html.find('>', i);
            if (end == std::string::npos) break;

            std::string_view tagContent = html.substr(i + 1, end - i - 1);
            if (tagContent.empty()) {
                i = end + 1;
                continue;
            }

            
            if (tagContent[0] == '/')
            {
                std::string tagName(tagContent.substr(1));
                
                tagName.erase(std::remove_if(tagName.begin(), tagName.end(), ::isspace), tagName.end());
                tokens.push_back({TokenType::TagClose, tagName, ""});
            } 
            else 
            {
                
                size_t spacePos = tagContent.find(' ');
                std::string tagName;
                std::string attr = "";

                if (spacePos == std::string_view::npos) {
                    tagName = std::string(tagContent);
                } else {
                    tagName = std::string(tagContent.substr(0, spacePos));
                    attr = std::string(tagContent.substr(spacePos + 1));
                }

                
                if (!attr.empty() && attr.back() == '/') {
                    attr.pop_back();
                }

                tokens.push_back({TokenType::TagOpen, tagName, attr});
            }
            i = end + 1;
        } 
        else 
        {
            
            size_t end = html.find('<', i);
            if (end == std::string::npos) end = html.length();

            std::string text(html.substr(i, end - i));
            
            
            if (text.find_first_not_of(" \n\r\t") != std::string::npos)
            {
                tokens.push_back({TokenType::Text, text, ""});
            }
            i = end;
        }
    }
    return tokens;
}
