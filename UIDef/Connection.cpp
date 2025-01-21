#include "Connection.h"

bool Connection::checkInternetConnection() {
    WSADATA wsaData;
    SOCKET socketTcp;
    struct sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    if ((socketTcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(53);

    //const char* address = "srv513883.hstgr.cloud";
    const char* address = "8.8.8.8";
    if (inet_pton(AF_INET, address, &server.sin_addr) <= 0) {
        closesocket(socketTcp);
        WSACleanup();
        return false;
    }

    int result = connect(socketTcp, (struct sockaddr*)&server, sizeof(server));
    closesocket(socketTcp);
    WSACleanup();

    return result == 0;
}

std::string Connection::sendPost(std::string url, nlohmann::json body) {
    
    std::wstring urlW(url.begin(), url.end());
    LPCWSTR urlLpcwstr = urlW.c_str();
    const char* headers = "Content-Type: application/json\r\n";
    std::string jsonString = body.dump();
    DWORD dwBytesWritten = 0;

    HINTERNET hInternet = InternetOpen(L"HTTP Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return "";
    }
    HINTERNET hConnect = InternetOpenUrlW(hInternet, urlLpcwstr, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    size_t sizeJson = jsonString.length();
    DWORD dwJson = static_cast<DWORD>(sizeJson);
    BOOL result = HttpSendRequestA(hConnect, headers, -1, (LPVOID)jsonString.c_str(), dwJson);
    if (!result) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    DWORD buffer[4096];
    DWORD bytesRead;
    std::string response;
    while(InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        response.append(reinterpret_cast<char*>(buffer), bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return response;
}

size_t writeCurlCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);

    return totalSize;
}

std::string Connection::sendPostCurl(std::string url, nlohmann::json body) {
    
    CURL* curl;
    CURLcode curlCode;

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string jsonString = body.dump();
    std::string readBuffer, response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCurlCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
        
        curlCode = curl_easy_perform(curl);

        if (curlCode != CURLE_OK) {
            //curl_easy_strerror(curlCode);
        }
        else {
            response = readBuffer;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return response;
}