#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <sstream>
#include "CssEngine.h"
#include "Terminal.h"

class FormComponent;


inline int getUtf8Length(const std::string& str) {
    int len = 0;
    for (size_t i = 0; i < str.length(); ) {
        int c = (unsigned char)str[i];
        if (c >= 0 && c <= 127) i += 1;
        else if ((c & 0xE0) == 0xC0) i += 2;
        else if ((c & 0xF0) == 0xE0) i += 3;
        else if ((c & 0xF8) == 0xF0) i += 4;
        else i += 1; 
        len++;
    }
    return len;
}

class UIWidget {
protected:
    bool focusable;
    bool isFocused = false;
    StyleParams style;

public:
    UIWidget(bool f, StyleParams s = StyleParams()) : focusable(f), style(s) {}
    virtual ~UIWidget() = default;

    bool isFocusable() const { return focusable; }
    void setFocus(bool f) { isFocused = f; }
    
    virtual bool isInput() const { return false; }
    
    virtual void draw(int termWidth) const = 0;
    virtual std::string action() = 0;
    
    virtual void appendChar(char c) {}
    virtual void backspace() {}
};

class TextWidget : public UIWidget {
private:
    std::string text;
public:
    TextWidget(const std::string& t, StyleParams s = StyleParams()) : UIWidget(false, s), text(t) {}
    
    void draw(int termWidth) const override {
        if (text == "\n") { 
            std::cout << "\033[K\n"; 
            return; 
        }
        
        std::cout << CssEngine::getColorCode(style.color) << CssEngine::getBgCode(style.bg) 
                  << text << "\033[0m\033[K\n"; 
    }
    
    std::string action() override { return ""; }
};

class LinkWidget : public UIWidget {
private:
    std::string text, href;
public:
    LinkWidget(const std::string& t, const std::string& h, StyleParams s = StyleParams()) : UIWidget(true, s), text(t), href(h) {}
    
    void draw(int termWidth) const override {
        std::string fullText = "[" + text + "]";
        
        if (isFocused) {
            std::cout << "\033[44m\033[1m" << fullText << "\033[0m\033[K\n"; 
        } else {
            std::string textCol = style.color.empty() ? "blue" : style.color;
            std::cout << CssEngine::getColorCode(textCol) << CssEngine::getBgCode(style.bg) 
                      << "\033[4m" << fullText << "\033[0m\033[K\n";
        }
    }
    
    std::string action() override { return "NAVIGATE:" + href; }
};

class InputWidget : public UIWidget {
private:
    std::string placeholder;
    std::string currentText;
    std::string name;

public:
    InputWidget(const std::string& p, const std::string& n, StyleParams s = StyleParams()) 
        : UIWidget(true, s), placeholder(p), name(n) {}
    
    bool isInput() const override { return true; }

    std::string getName() const { return name; }
    std::string getValue() const { return currentText; }
    
    void appendChar(char c) override { currentText += c; }
    void backspace() override { if (!currentText.empty()) currentText.pop_back(); }

    void draw(int termWidth) const override {
        std::string display = currentText.empty() ? placeholder : currentText;
        std::string fullText = " [" + display + "] ";
        
        if (isFocused) {
            std::cout << "\033[47m\033[30m\033[1m" << fullText << "\033[0m \033[33m◀ Type text...\033[0m";
        } else {
            std::cout << "\033[48;5;240m\033[37m" << fullText << "\033[0m";
        }
        std::cout << "\033[K\n";
    }

    std::string action() override { return "REFRESH"; }
};

class FormComponent {
public:
    std::string actionUrl;
    std::vector<std::shared_ptr<InputWidget>> inputs;

    std::string submit() {
        if (actionUrl.empty()) actionUrl = "home";
        std::string target = actionUrl;
        
        bool first = true;
        for (const auto& in : inputs) {
            if (in->getName().empty()) continue;
            target += (first ? "?" : "&");
            target += in->getName() + "=" + in->getValue();
            first = false;
        }
        return "NAVIGATE:" + target;
    }
};

class ButtonWidget : public UIWidget {
private:
    std::string label;
    std::shared_ptr<FormComponent> parentForm;

public:
    ButtonWidget(const std::string& l, std::shared_ptr<FormComponent> form = nullptr, StyleParams s = StyleParams()) 
        : UIWidget(true, s), label(l), parentForm(form) {}

    void draw(int termWidth) const override {
        std::string fullText = "  " + label + "  ";
        
        if (isFocused) {
            std::cout << "\033[7m" << fullText << "\033[0m\033[K\n"; 
        } else {
            std::string bgCol = style.bg.empty() ? "white" : style.bg;
            std::string textCol = style.color.empty() ? "black" : style.color;
            std::cout << CssEngine::getColorCode(textCol) << CssEngine::getBgCode(bgCol) << fullText << "\033[0m\033[K\n";
        }
    }

    std::string action() override { 
        if (parentForm) return parentForm->submit();
        return ""; 
    }
};

class PageDOM {
private:
    std::vector<std::shared_ptr<UIWidget>> widgets;
    int currentFocusIndex = -1; 
    std::string overlayUrlBuffer; 

    void validateFocus(bool canBack, bool canForward) {
        if (currentFocusIndex == 0 && canBack) return;
        if (currentFocusIndex == 1 && canForward) return;
        if (currentFocusIndex == 2) return; 
        if (currentFocusIndex >= 3 && currentFocusIndex - 3 < (int)widgets.size()) {
            if (widgets[currentFocusIndex - 3]->isFocusable()) return;
        }

        for (size_t i = 0; i < widgets.size(); ++i) {
            if (widgets[i]->isFocusable()) {
                currentFocusIndex = (int)i + 3;
                widgets[i]->setFocus(true);
                return;
            }
        }
        currentFocusIndex = 2; 
    }

public:
    bool isInputFocused() {
        if (currentFocusIndex >= 3 && currentFocusIndex - 3 < (int)widgets.size()) {
            return widgets[currentFocusIndex - 3]->isInput();
        }
        return false;
    }
    
    void clear() { widgets.clear(); currentFocusIndex = -1; overlayUrlBuffer.clear(); }
    
    void addWidget(std::shared_ptr<UIWidget> w) {
        widgets.push_back(w);
    }

    bool isOverlayUrlFocused() const {
        return currentFocusIndex == 2;
    }

    std::string getOverlayUrlValue() const {
        return overlayUrlBuffer;
    }

    void setOverlayUrlValue(const std::string& url) {
        overlayUrlBuffer = url;
    }

    void appendOverlayUrlChar(char c) {
        overlayUrlBuffer += c;
    }

    void backspaceOverlayUrl() {
        if (!overlayUrlBuffer.empty()) {
            overlayUrlBuffer.pop_back();
        }
    }

    std::shared_ptr<UIWidget> getFocusedWidget() {
        if (currentFocusIndex >= 3 && currentFocusIndex - 3 < (int)widgets.size()) {
            return widgets[currentFocusIndex - 3];
        }
        return nullptr;
    }

    void drawPage(const std::string& currentUrl, bool canBack, bool canForward) {
        validateFocus(canBack, canForward);
        
        if (overlayUrlBuffer.empty() && currentFocusIndex != 2) {
            overlayUrlBuffer = currentUrl;
        }
        
        std::cout << "\033[H"; 
        int termWidth = Terminal::getWidth(); 

        std::cout << "\033[48;5;235m\033[37m"; 

        if (!canBack) {
            std::cout << "\033[90m [Back]       \033[37m\033[48;5;235m";
        } else if (currentFocusIndex == 0) {
            std::cout << "\033[46;30m [< Back (b)] \033[37m\033[48;5;235m"; 
        } else {
            std::cout << "\033[92m [< Back (b)] \033[37m\033[48;5;235m"; 
        }

        if (!canForward) {
            std::cout << "\033[90m [Forward]      \033[37m\033[48;5;235m";
        } else if (currentFocusIndex == 1) {
            std::cout << "\033[46;30m [Forward > (f)] \033[37m\033[48;5;235m";
        } else {
            std::cout << "\033[92m [Forward > (f)] \033[37m\033[48;5;235m";
        }

        std::cout << "  Address: ";
        int usedWidth = 41; 
        
        if (currentFocusIndex == 2) {
            std::string displayStr = " [" + overlayUrlBuffer + "] ◀ (Press Enter to go) ";
            std::cout << "\033[47m\033[30m\033[1m" << displayStr << "\033[0m\033[48;5;235m\033[37m";
            usedWidth += getUtf8Length(displayStr);
        } else {
            std::string displayStr = " [" + overlayUrlBuffer + "] ";
            std::cout << "\033[48;5;240m\033[37m" << displayStr << "\033[0m\033[48;5;235m\033[37m";
            usedWidth += getUtf8Length(displayStr);
        }
        
        if (usedWidth < termWidth) {
            std::cout << std::string(termWidth - usedWidth, ' ');
        }
        std::cout << "\033[0m\033[K\n"; 
        std::cout << "\033[34m" << std::string(termWidth, '-') << "\033[0m\033[K\n\n";
        
        for (const auto& w : widgets) w->draw(termWidth);
        
        std::cout << "\033[J"; 
        
        std::cout << "\n\n====================================================\033[K\n";
        std::cout << "[Arrows/Tab]: Move | [Just Type]: Text Inputs | [Enter]: Action | [q]: Exit\033[K\n";
    }

    void nextFocus(bool canBack, bool canForward) {
        int totalItems = 3 + widgets.size(); 
        
        if (currentFocusIndex >= 3 && currentFocusIndex - 3 < (int)widgets.size()) {
            widgets[currentFocusIndex - 3]->setFocus(false);
        }

        int start = currentFocusIndex == -1 ? 0 : currentFocusIndex;
        do {
            currentFocusIndex = (currentFocusIndex + 1) % totalItems;
            if (currentFocusIndex == 0 && canBack) return;
            if (currentFocusIndex == 1 && canForward) return;
            if (currentFocusIndex == 2) return; 
            if (currentFocusIndex >= 3) {
                if (widgets[currentFocusIndex - 3]->isFocusable()) {
                    widgets[currentFocusIndex - 3]->setFocus(true);
                    return;
                }
            }
        } while (currentFocusIndex != start);
    }

    void prevFocus(bool canBack, bool canForward) {
        int totalItems = 3 + widgets.size();

        if (currentFocusIndex >= 3 && currentFocusIndex - 3 < (int)widgets.size()) {
            widgets[currentFocusIndex - 3]->setFocus(false);
        }

        int start = currentFocusIndex == -1 ? 0 : currentFocusIndex;
        do {
            currentFocusIndex = (currentFocusIndex - 1 + totalItems) % totalItems;
            if (currentFocusIndex == 0 && canBack) return;
            if (currentFocusIndex == 1 && canForward) return;
            if (currentFocusIndex == 2) return;
            if (currentFocusIndex >= 3) {
                if (widgets[currentFocusIndex - 3]->isFocusable()) {
                    widgets[currentFocusIndex - 3]->setFocus(true);
                    return;
                }
            }
        } while (currentFocusIndex != start);
    }

    std::string triggerActive() {
        if (currentFocusIndex == 0) return "BROWSER_BACK";
        if (currentFocusIndex == 1) return "BROWSER_FORWARD";
        if (currentFocusIndex == 2) return "BROWSER_GOTO_URL"; 
        if (currentFocusIndex >= 3 && currentFocusIndex - 3 < (int)widgets.size()) {
            return widgets[currentFocusIndex - 3]->action();
        }
        return "";
    }
};
