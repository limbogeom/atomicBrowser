#include "Browser.h"
#include "HtmlLexer.h"
#include "Terminal.h"
#include <iostream>
#include <algorithm>
#include <fstream>





static std::string extractAttribute(const std::string& attrStr, const std::string& attrName) {
    size_t pos = attrStr.find(attrName + "=\"");
    if (pos == std::string::npos) {
        pos = attrStr.find(attrName + "='");
        if (pos == std::string::npos) return "";
        size_t start = pos + attrName.length() + 2;
        size_t end = attrStr.find('\'', start);
        if (end == std::string::npos) return "";
        return attrStr.substr(start, end - start);
    }
    size_t start = pos + attrName.length() + 2;
    size_t end = attrStr.find('"', start);
    if (end == std::string::npos) return "";
    return attrStr.substr(start, end - start);
}

static std::string cleanGoogleUrl(const std::string& rawUrl) {
    if (rawUrl.find("/url?q=") == 0) {
        size_t start = 7; 
        size_t end = rawUrl.find("&", start);
        std::string extracted = (end == std::string::npos) ? rawUrl.substr(start) : rawUrl.substr(start, end - start);
        
        size_t p;
        while ((p = extracted.find("%3A")) != std::string::npos) extracted.replace(p, 3, ":");
        while ((p = extracted.find("%2F")) != std::string::npos) extracted.replace(p, 3, "/");
        while ((p = extracted.find("%3F")) != std::string::npos) extracted.replace(p, 3, "?");
        while ((p = extracted.find("%3D")) != std::string::npos) extracted.replace(p, 3, "=");
        while ((p = extracted.find("%26")) != std::string::npos) extracted.replace(p, 3, "&");
        return extracted;
    }
    return rawUrl;
}

static std::string decodeHtmlEntities(std::string text) {
    size_t p;
    while ((p = text.find("&quot;")) != std::string::npos) text.replace(p, 6, "\"");
    while ((p = text.find("&amp;")) != std::string::npos)  text.replace(p, 5, "&");
    while ((p = text.find("&lt;")) != std::string::npos)   text.replace(p, 4, "<");
    while ((p = text.find("&gt;")) != std::string::npos)   text.replace(p, 4, ">");
    while ((p = text.find("&#39;")) != std::string::npos)  text.replace(p, 5, "'");
    while ((p = text.find("&nbsp;")) != std::string::npos) text.replace(p, 6, " ");
    return text;
}





PageDOM Browser::buildDOM(const std::vector<Token>& tokens) {
    PageDOM dom;
    
    bool inLink = false, inButton = false;
    bool inBold = false, inItalic = false;
    bool inBlockquote = false;
    bool inOrderedList = false;
    
    int listCounter = 1; 
    std::string currentHref = "";
    StyleParams currentInlineStyle;

    std::shared_ptr<FormComponent> activeForm = nullptr;
    cssEngine.clear();

    
    bool inStyleTag = false;
    for (const auto& t : tokens) {
        if (t.type == TokenType::TagOpen && t.value == "style") inStyleTag = true;
        else if (t.type == TokenType::TagClose && t.value == "style") inStyleTag = false;
        else if (t.type == TokenType::Text && inStyleTag) {
            cssEngine.parseStyleTag(t.value);
        }
    }

    bool skipContent = false;
    
    std::vector<bool> displayNoneStack; 
    
    std::string currentLineBuffer = "";
    StyleParams currentLineStyle = cssEngine.getStyleForTag("p");

    auto flushBuffer = [&]() {
        if (!currentLineBuffer.empty()) {
            if (currentLineBuffer.find_first_not_of(" \n\r\t") != std::string::npos || currentLineBuffer.find("\033") != std::string::npos) {
                dom.addWidget(std::make_shared<TextWidget>(currentLineBuffer, currentLineStyle));
            }
            currentLineBuffer = "";
        }
    };

    
    for (const auto& t : tokens) {
        std::string tag = t.value;
        std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);

        if (t.type == TokenType::TagOpen) {
            std::string styleAttr = extractAttribute(t.attr, "style");
            StyleParams itemStyle = CssEngine::parseInlineStyle(styleAttr, cssEngine.getStyleForTag(tag));

            
            
            if (itemStyle.display == "none" || (!displayNoneStack.empty() && displayNoneStack.back())) {
                displayNoneStack.push_back(true);
            } else {
                displayNoneStack.push_back(false);
            }

            if (tag == "script" || tag == "style" || tag == "svg" || tag == "noscript" || tag == "head" || tag == "meta" || tag == "iframe") {
                skipContent = true;
            }
            else if (!skipContent && !displayNoneStack.back()) { 
                
                
                
                if (itemStyle.display == "block" && tag != "table" && tag != "tr") {
                    flushBuffer();
                    currentLineStyle = itemStyle;
                }

                if (tag == "form") {
                    flushBuffer();
                    activeForm = std::make_shared<FormComponent>();
                    activeForm->actionUrl = extractAttribute(t.attr, "action");
                }
                else if (tag == "a") { 
                    flushBuffer(); 
                    inLink = true; 
                    currentHref = cleanGoogleUrl(extractAttribute(t.attr, "href")); 
                    currentInlineStyle = itemStyle; 
                }
                else if (tag == "button") { 
                    flushBuffer(); 
                    inButton = true; 
                    currentInlineStyle = itemStyle; 
                }
                else if (tag == "input") {
                    flushBuffer(); 
                    std::string ph = extractAttribute(t.attr, "placeholder");
                    std::string name = extractAttribute(t.attr, "name");
                    auto inputWidget = std::make_shared<InputWidget>(
                        ph.empty() ? "Type a query..." : ph, 
                        name.empty() ? "q" : name, 
                        itemStyle
                    );
                    if (activeForm) activeForm->inputs.push_back(inputWidget);
                    dom.addWidget(inputWidget);
                }
                else if (tag == "b" || tag == "strong") inBold = true;
                else if (tag == "i" || tag == "em") inItalic = true;
                else if (tag == "blockquote") {
                    flushBuffer();
                    inBlockquote = true;
                    currentLineBuffer += "  │ \033[3m"; 
                    currentLineStyle = itemStyle;
                }
                else if (tag == "ol") { flushBuffer(); inOrderedList = true; listCounter = 1; }
                else if (tag == "ul") { flushBuffer(); inOrderedList = false; }
                else if (tag == "li") {
                    flushBuffer();
                    if (inOrderedList) currentLineBuffer += "  " + std::to_string(listCounter++) + ". ";
                    else currentLineBuffer += "  • ";
                    currentLineStyle = itemStyle;
                }
                else if (tag == "hr") {
                    flushBuffer();
                    int termWidth = Terminal::getWidth();
                    std::string hrLine = std::string(termWidth > 20 ? termWidth - 4 : 40, '-');
                    dom.addWidget(std::make_shared<TextWidget>(hrLine, itemStyle));
                }
                
                else if (tag == "table") {
                    flushBuffer();
                }
                else if (tag == "tr") {
                    flushBuffer(); 
                    currentLineBuffer += "   "; 
                }
                else if (tag == "th" || tag == "td") {
                    currentLineBuffer += " │ "; 
                }
                else if (tag == "br") {
                    flushBuffer();
                }
                else if (tag == "h1") { flushBuffer(); currentLineBuffer += "\033[1;36m=== "; currentLineStyle = itemStyle; }
                else if (tag == "h2") { flushBuffer(); currentLineBuffer += "\033[1;32m-- "; currentLineStyle = itemStyle; }
                else if (tag == "h3") { flushBuffer(); currentLineBuffer += "\033[1;33m- "; currentLineStyle = itemStyle; }
            }
        } 
        else if (t.type == TokenType::TagClose) {
            bool wasHidden = !displayNoneStack.empty() && displayNoneStack.back();
            if (!displayNoneStack.empty()) {
                displayNoneStack.pop_back(); 
            }

            if (tag == "script" || tag == "style" || tag == "svg" || tag == "noscript" || tag == "head" || tag == "meta" || tag == "iframe") {
                skipContent = false; 
            }
            else if (!skipContent && !wasHidden) { 
                
                StyleParams itemStyle = cssEngine.getStyleForTag(tag);
                
                if (tag == "form") activeForm = nullptr;
                else if (tag == "a") { flushBuffer(); inLink = false; }
                else if (tag == "button") { flushBuffer(); inButton = false; }
                else if (tag == "b" || tag == "strong") inBold = false;
                else if (tag == "i" || tag == "em") inItalic = false;
                else if (tag == "blockquote") {
                    currentLineBuffer += "\033[0m"; 
                    flushBuffer();
                    inBlockquote = false;
                }
                else if (tag == "th" || tag == "td") {
                    currentLineBuffer += " "; 
                }
                else if (tag == "tr") {
                    currentLineBuffer += " │"; 
                    flushBuffer();
                }
                else if (tag == "h1") { currentLineBuffer += " ===\033[0m"; flushBuffer(); }
                else if (tag == "h2") { currentLineBuffer += " --\033[0m"; flushBuffer(); }
                else if (tag == "h3") { currentLineBuffer += " -\033[0m"; flushBuffer(); }
                
                
                else if (itemStyle.display == "block" || tag == "table") {
                    flushBuffer(); 
                }
            }
        } 
        else if (t.type == TokenType::Text && !skipContent) {
            
            if (!displayNoneStack.empty() && displayNoneStack.back()) {
                continue;
            }

            if (t.value.find_first_not_of(" \n\r\t") == std::string::npos) continue;

            std::string cleanText = decodeHtmlEntities(t.value);
            cleanText.erase(std::remove(cleanText.begin(), cleanText.end(), '\r'), cleanText.end());
            std::replace(cleanText.begin(), cleanText.end(), '\n', ' ');

            std::string formattingPrefix = "";
            std::string formattingSuffix = "";

            if (inBold) { formattingPrefix += "\033[1m"; formattingSuffix += "\033[22m"; }
            if (inItalic) { formattingPrefix += "\033[3m"; formattingSuffix += "\033[23m"; }

            std::string finalOutput = formattingPrefix + cleanText + formattingSuffix;

            if (inLink) {
                if (!currentHref.empty() && currentHref.find("javascript:") != 0) {
                    dom.addWidget(std::make_shared<LinkWidget>(finalOutput, currentHref, currentInlineStyle));
                } else {
                    currentLineBuffer += finalOutput + " ";
                }
            }
            else if (inButton) {
                dom.addWidget(std::make_shared<ButtonWidget>(finalOutput, activeForm, currentInlineStyle));
            }
            else {
                currentLineBuffer += finalOutput;
            }
        }
    }
    
    flushBuffer(); 
    return dom;
}

void Browser::loadUrl(const std::string& url) {
    int termWidth = Terminal::getWidth();
    if (termWidth <= 0) termWidth = 80;

    if (history.empty() || currentDOM.getOverlayUrlValue() != url) {
        std::cout << "\033[2J\033[H" << std::flush; 
        std::cout << "\033[48;5;238m\033[37m"; 
        std::string loadTitle = " [ SimpleBrowser ]  Loading: " + (url.empty() ? "home" : url);
        if ((int)loadTitle.length() < termWidth) {
            loadTitle += std::string(termWidth - loadTitle.length(), ' ');
        }
        std::cout << loadTitle << "\033[0m\033[K\n";
        std::cout << "\033[33m" << std::string(termWidth, '-') << "\033[0m\033[K\n\n";
        std::cout << "  \033[1;36mProcessing request...\033[0m\033[K\n";
        std::cout << "  \033[32m[▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░]\033[0m Analyzing data source...\033[K\n";
        std::cout << "\033[J" << std::flush;
    }

    std::string html;

    if (url == "home" || url == "") {
        html = 
            "<style>"
            "  h1 { color: cyan; }"
            "  h2 { color: green; }"
            "  h3 { color: yellow; }"
            "  p { color: white; }"
            "</style>"
            "<h1>WELCOME TO SimpleBrowser!</h1>"
            "<p>This main index page demonstrates every HTML rendering capability currently supported by your engine.</p>"
            "<hr>"
            
            "<h2>1. Typography & Text Formatting</h2>"
            "<h1>This is a Header 1 (h1)</h1>"
            "<h2>This is a Header 2 (h2)</h2>"
            "<h3>This is a Header 3 (h3)</h3>"
            "<p>This is standard paragraph text (p). Here is an example of <b>bold text using the b tag</b>, and <strong>strongly emphasized text using strong</strong>.</p>"
            "<p>You can also use <i>italic text (i tag)</i> or <em>emphasized text (em tag)</em>.</p>"
            "<br>"
            
            "<h2>2. Layout Structures & Lists</h2>"
            "<blockquote>This is a blockquote element. It automatically generates a distinct vertical border line on the left side to gracefully format nested quotes.</blockquote>"
            "<p>Unordered List (ul + li):</p>"
            "<ul>"
            "  <li>Bullet element list node A</li>"
            "  <li>Bullet element list node B</li>"
            "</ul>"
            "<p>Ordered List (ol + li):</p>"
            "<ol>"
            "  <li>Step number one in sequence</li>"
            "  <li>Step number two in sequence</li>"
            "  <li>Step number three in sequence</li>"
            "</ol>"
            "<hr>"
            
            "<h2>3. Table Formatting</h2>"
            "<table>"
            "  <tr>"
            "    <th>ID</th>"
            "    <th>Component Name</th>"
            "    <th>Engine Status</th>"
            "  </tr>"
            "  <tr>"
            "    <td>01</td>"
            "    <td>Lexer & Tokenizer</td>"
            "    <td>Operational</td>"
            "  </tr>"
            "  <tr>"
            "    <td>02</td>"
            "    <td>Raw Input Handling</td>"
            "    <td>Stable</td>"
            "  </tr>"
            "</table>"
            "<br><hr>"
            
            "<h2>4. Interactive Forms & Inputs</h2>"
            "<p>Authentication Form simulation sandbox (navigates to gmail_mock):</p>"
            "<form action=\"gmail_mock\">"
            "  <input name=\"user\" placeholder=\"Username (Email)\">"
            "  <input name=\"pass\" placeholder=\"Password\">"
            "  <br>"
            "  <button type=\"submit\">Login Securely</button>"
            "</form>"
            "<hr>"
            
            "<h2>5. Navigational Links</h2>"
            "<p>Use Tab to switch focus between interactive hyperlinked anchors:</p>"
            "<li><a href=\"gmail_mock\">Visit Profile Management Hub (Mock)</a></li>"
            "<li><a href=\"https://example.com\">Fetch External Website Example (example.com)</a></li>"
            "<br>"
            "<p style=\"color: gray; text-align: center;\">Controls: [TAB / Arrow Keys] Move Selection | [Enter] Trigger Action | [q] Exit Application</p>";
    }
    else if (url == "gmail_mock") {
        html = "<h1>Mock User Account Profile</h1>"
               "<p>Form variables successfully packaged and dispatched to destination route!</p>"
               "<input name='user' placeholder='Username'><br>"
               "<input name='pass' placeholder='Password'><br><br>"
               "<a href='home'>◀ Go Back Home</a>";
    }
    else {
        
        std::ifstream localFile(url);
        if (localFile.is_open()) {
            
            std::stringstream buffer;
            buffer << localFile.rdbuf();
            html = buffer.str();
            localFile.close();
        } 
        else {
            
            try {
                html = net.fetch(url);
            } catch (...) {
                html = "";
            }
        }

        
        if (html.empty() || html.find_first_not_of(" \n\r\t") == std::string::npos) {
            html = "<style>h1 { color: red; } p { color: yellow; }</style>"
                   "<h1>Resource Load Failed!</h1>"
                   "<p>Unable to retrieve resources from: " + url + "</p>"
                   "<p>Possible issues:</p>"
                   "<li>1. Local file path is invalid, misspelled, or lacks read permissions.</li>"
                   "<li>2. Target server forces SSL/HTTPS layers, which are unsupported by the socket library.</li>"
                   "<li>3. Destination address domain is unreachable or offline.</li>"
                   "<br><br><a href='home'>◀ Return Home</a>";
        }
    }

    currentDOM.clear();
    currentDOM = buildDOM(HtmlLexer::tokenize(html));
    currentDOM.setOverlayUrlValue(url.empty() ? "home" : url);
}

void Browser::navigate(const std::string& url) {
    std::string target = url;
    bool isLocalText = false;

    
    size_t dotPos = target.rfind('.');
    if (dotPos != std::string::npos) {
        std::string ext = target.substr(dotPos); 
        
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == ".html" || ext == ".htm" || ext == ".txt") {
            isLocalText = true;
        }
    }

    
    if (!isLocalText) {
        if (target.find("./") == 0 || target.find("../") == 0 || target.find("/") == 0 || 
            (target.length() > 2 && target[1] == ':' && target[2] == '/')) {
            isLocalText = true;
        }
    }

    
    if (!isLocalText) {
        std::ifstream testFile(target);
        if (testFile.is_open()) {
            isLocalText = true;
            testFile.close();
        }
    }

    
    if (!isLocalText && target != "home" && target != "" && 
        target.find("http://") != 0 && target.find("https://") != 0 && target.find("gmail_mock") != 0) {
        target = "https://" + target;
    }

    if (currentHistoryIndex < (int)history.size() - 1) {
        history.resize(currentHistoryIndex + 1);
    }
    history.push_back(target);
    currentHistoryIndex++;
    loadUrl(target);
}

void Browser::goBack() {
    if (currentHistoryIndex > 0) {
        currentHistoryIndex--;
        loadUrl(history[currentHistoryIndex]);
    }
}

void Browser::goForward() {
    if (currentHistoryIndex < (int)history.size() - 1) {
        currentHistoryIndex++;
        loadUrl(history[currentHistoryIndex]);
    }
}

void Browser::runInteractiveLoop() {
    std::cout << "\033[2J\033[H" << std::flush; 
    navigate("home");

    Terminal::enableRawMode(); 

    while (true) {
        std::string currentUrl = history[currentHistoryIndex];
        bool canBack = currentHistoryIndex > 0;
        bool canForward = currentHistoryIndex < (int)history.size() - 1;
        
        currentDOM.drawPage(currentUrl, canBack, canForward);
        int key = Terminal::readKey();
        
        
        if (currentDOM.isOverlayUrlFocused()) {
            if (key == Terminal::ENTER) {
                std::string targetUrl = currentDOM.getOverlayUrlValue();
                if (!targetUrl.empty()) {
                    navigate(targetUrl);
                }
            }
            else if (key == Terminal::BACKSPACE || key == 127) {
                currentDOM.backspaceOverlayUrl();
            }
            else if (key == Terminal::TAB || key == Terminal::DOWN) {
                currentDOM.nextFocus(canBack, canForward);
            }
            else if (key == Terminal::UP) {
                currentDOM.prevFocus(canBack, canForward);
            }
            else if (key >= 32 && key <= 126) {
                currentDOM.appendOverlayUrlChar((char)key);
            }
        }
        
        else if (currentDOM.isInputFocused()) {
            auto focusedWidget = currentDOM.getFocusedWidget();
            
            if (key == Terminal::ENTER) {
                std::string actionCmd = currentDOM.triggerActive();
                if (actionCmd.find("NAVIGATE:") == 0) {
                    navigate(actionCmd.substr(9));
                }
            }
            else if (key == Terminal::BACKSPACE || key == 127) {
                if (focusedWidget) focusedWidget->backspace();
            }
            else if (key == Terminal::TAB || key == Terminal::DOWN) {
                currentDOM.nextFocus(canBack, canForward);
            }
            else if (key == Terminal::UP) {
                currentDOM.prevFocus(canBack, canForward);
            }
            else if (key >= 32 && key <= 126) {
                if (focusedWidget) focusedWidget->appendChar((char)key); 
            }
        } 
        
        else {
            if (key == 'q' || key == 'Q') {
                break; 
            }
            else if (key == 'b' || key == 'B') {
                goBack();
            }
            else if (key == 'f' || key == 'F') {
                goForward();
            }
            else if (key == Terminal::TAB || key == Terminal::RIGHT || key == Terminal::DOWN) {
                currentDOM.nextFocus(canBack, canForward); 
            }
            else if (key == Terminal::LEFT || key == Terminal::UP) {
                currentDOM.prevFocus(canBack, canForward); 
            }
            else if (key == Terminal::ENTER) {
                std::string actionCmd = currentDOM.triggerActive();
                if (actionCmd == "BROWSER_BACK") {
                    goBack();
                }
                else if (actionCmd == "BROWSER_FORWARD") {
                    goForward();
                }
                else if (actionCmd.find("NAVIGATE:") == 0) {
                    navigate(actionCmd.substr(9));
                }
            }
        }
    }
    Terminal::disableRawMode(); 
    std::cout << "\033[2J\033[H" << std::flush; 
}
