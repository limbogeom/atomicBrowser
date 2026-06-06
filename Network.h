#pragma once
#include <string>
#include <unordered_map>

class Network
{
private:
    std::unordered_map<std::string, std::string> cache;

public:
    std::string fetch(const std::string& url);
};