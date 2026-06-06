cmake_minimum_required(VERSION 3.10)
project(SimpleBrowser)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)

add_executable(browser
    main.cpp
    Network.cpp
    HtmlLexer.cpp
    Renderer.cpp
    Browser.cpp)

target_link_libraries(browser CURL::libcurl)