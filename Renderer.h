#pragma once
#include "HtmlLexer.h"
#include <string>
#include <vector>

class Renderer
{
public:
    static void render(const std::vector<Token>& tokens);
};