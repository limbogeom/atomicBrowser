#include "Network.h"
#include <curl/curl.h>
#include <iostream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string Network::fetch(const std::string& url)
{
    if (cache.find(url) != cache.end())
    {
        std::cout << "Loaded from cache...\n";
        return cache[url];
    }

    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl)
    {
        std::string fullUrl = (url.find("http") == 0) ? url : "https://" + url;

        std::string userAgent = "SimpleBrowser/2.0 libcurl/7.x";

        curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            readBuffer = "<h1>Network error</h1><p>Can't load: " + url + "</p>";
        } else 
        {
            cache[url] = readBuffer;
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}
