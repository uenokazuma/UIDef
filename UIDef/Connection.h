#pragma once
#include <iostream>
//#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")

class Connection {
public:
    bool checkInternetConnection();
    std::string sendPost(std::string url, nlohmann::json json);
    std::string sendPostCurl(std::string url, nlohmann::json json);
};