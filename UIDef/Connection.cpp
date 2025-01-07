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

    const char* dns = "8.8.8.8";
    if (inet_pton(AF_INET, dns, &server.sin_addr) <= 0) {
        closesocket(socketTcp);
        WSACleanup();
        return false;
    }

    int result = connect(socketTcp, (struct sockaddr*)&server, sizeof(server));
    closesocket(socketTcp);
    WSACleanup();

    return result == 0;
}