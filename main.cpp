#include "Browser.h"
#include <iostream>

int main()
{
    Browser browser;
    browser.runInteractiveLoop();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}