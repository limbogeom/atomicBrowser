#pragma once
#include "Network.h"
#include "DOM.h"
#include "HtmlLexer.h"
#include "CssEngine.h"
#include <string>
#include <vector>

class Browser {
private:
    Network net;
    std::vector<std::string> history;
    int currentHistoryIndex = -1;
    PageDOM currentDOM;
    CssEngine cssEngine;

    PageDOM buildDOM(const std::vector<Token>& tokens);
    void loadUrl(const std::string& url); 

public:
    void navigate(const std::string& url);
    void goBack();
    void goForward();
    void runInteractiveLoop();
};
