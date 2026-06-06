#pragma once
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <iostream>


struct StyleParams {
    std::string color = "";
    std::string bg = "";
    std::string textAlign = "left"; 
    std::string display = "inline";  
    int marginLeft = 0;
    int marginRight = 0;
};

class CssEngine {
private:
    
    std::unordered_map<std::string, StyleParams> globalStyles;

    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }

    
    void initDefaultStyles() {
        globalStyles["div"].display = "block";
        globalStyles["p"].display = "block";
        globalStyles["h1"].display = "block";
        globalStyles["h2"].display = "block";
        globalStyles["h3"].display = "block";
        globalStyles["table"].display = "block";
        globalStyles["form"].display = "block";
        globalStyles["blockquote"].display = "block";
        globalStyles["ul"].display = "block";
        globalStyles["ol"].display = "block";
        globalStyles["li"].display = "block";
    }

public:
    CssEngine() {
        initDefaultStyles();
    }

    
    static StyleParams parseInlineStyle(const std::string& styleStr, StyleParams base = StyleParams()) {
        StyleParams style = base;
        std::stringstream ss(styleStr);
        std::string item;
        while (std::getline(ss, item, ';')) {
            size_t colon = item.find(':');
            if (colon == std::string::npos) continue;
            std::string key = trim(item.substr(0, colon));
            std::string value = trim(item.substr(colon + 1));

            if (key == "color") style.color = value;
            else if (key == "bg" || key == "background") style.bg = value;
            else if (key == "text-align") style.textAlign = value;
            else if (key == "display") style.display = value; 
            else if (key == "margin-left") {
                try { style.marginLeft = std::stoi(value); } catch (...) {}
            }
            else if (key == "margin-right") {
                try { style.marginRight = std::stoi(value); } catch (...) {}
            }
        }
        return style;
    }

    
    void parseStyleTag(const std::string& cssContent) {
        size_t pos = 0;
        while (pos < cssContent.length()) {
            size_t braceOpen = cssContent.find('{', pos);
            if (braceOpen == std::string::npos) break;

            std::string selector = trim(cssContent.substr(pos, braceOpen - pos));
            size_t braceClose = cssContent.find('}', braceOpen);
            if (braceClose == std::string::npos) break;

            std::string rules = cssContent.substr(braceOpen + 1, braceClose - braceOpen - 1);
            
            
            StyleParams baseParam;
            if (globalStyles.find(selector) != globalStyles.end()) {
                baseParam = globalStyles[selector];
            } else {
                
                if (selector == "div" || selector == "p" || selector == "h1" || 
                    selector == "h2" || selector == "h3" || selector == "table" || 
                    selector == "form" || selector == "blockquote" || selector == "ul" || 
                    selector == "ol" || selector == "li") {
                    baseParam.display = "block";
                }
            }

            globalStyles[selector] = parseInlineStyle(rules, baseParam);
            pos = braceClose + 1;
        }
    }

    StyleParams getStyleForTag(const std::string& tagName) {
        if (globalStyles.find(tagName) != globalStyles.end()) {
            return globalStyles[tagName];
        }
        
        
        StyleParams defaultStyle;
        if (tagName == "div" || tagName == "p" || tagName == "h1" || 
            tagName == "h2" || tagName == "h3" || tagName == "table" || 
            tagName == "form" || tagName == "blockquote" || tagName == "ul" || 
            tagName == "ol" || tagName == "li") {
            defaultStyle.display = "block";
        }
        return defaultStyle;
    }

    void clear() {
        globalStyles.clear();
        initDefaultStyles(); 
    }

    static std::string getColorCode(const std::string& colorName) {
        if (colorName == "red") return "\033[31m";
        if (colorName == "green") return "\033[32m";
        if (colorName == "blue") return "\033[34m";
        if (colorName == "yellow") return "\033[33m";
        if (colorName == "black") return "\033[30m";
        if (colorName == "white") return "\033[37m";
        return "";
    }

    static std::string getBgCode(const std::string& bgName) {
        if (bgName == "red") return "\033[41m";
        if (bgName == "green") return "\033[42m";
        if (bgName == "blue") return "\033[44m";
        if (bgName == "yellow") return "\033[43m";
        if (bgName == "white") return "\033[47m";
        if (bgName == "black") return "\033[40m";
        return "";
    }

    static void applyAlignmentAndMargin(const StyleParams& style, int textLength, int termWidth) {
        int paddingLeft = style.marginLeft;
        if (style.textAlign == "center") {
            paddingLeft += (termWidth - textLength) / 2;
        } else if (style.textAlign == "right") {
            paddingLeft += termWidth - textLength - style.marginRight;
        }
        if (paddingLeft > 0) {
            std::cout << std::string(paddingLeft, ' ');
        }
    }
};
