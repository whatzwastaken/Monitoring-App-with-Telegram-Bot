#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <ctime>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

bool isServerAvailable(const std::string& address) {
    std::string command = "ping -n 1 -w 1000 " + address + " > nul";
    return system(command.c_str()) == 0;
}

void sendNotification(const std::string& server_ip) {
    HINTERNET hSession = WinHttpOpen(L"Server Monitor/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        HINTERNET hConnect = WinHttpConnect(hSession, L"127.0.0.1", 5000, 0);
        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/notify", NULL, WINHTTP_NO_REFERER,
                                                    WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            if (hRequest) {
                std::string jsonData = "{\"server\": \"" + server_ip + "\"}";
                WinHttpSendRequest(hRequest, L"Content-Type: application/json\r\n", -1,
                                   (LPVOID)jsonData.c_str(), jsonData.size(),
                                   jsonData.size(), 0);
                WinHttpReceiveResponse(hRequest, NULL);
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }
}

void logUnavailableServer(const std::string& server, const std::string& ip) {
    std::ofstream logFile("unavailable_servers.log", std::ios::app);
    if (logFile.is_open()) {
        std::time_t now = std::time(nullptr);
        logFile << "Server " << server << " (" << ip << ") unavailable at " << std::ctime(&now);
        logFile.close();
    } else {
        std::cerr << "Failed to open log file." << std::endl;
    }
    sendNotification(server);
}

std::vector<std::string> loadServers(const std::string& filename) {
    std::vector<std::string> servers;
    std::ifstream file(filename);
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (!line.empty()) {
                servers.push_back(line);
            }
        }
        file.close();
    } else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }
    return servers;
}

void monitorServers(const std::vector<std::string>& servers) {
    while (true) {
        for (const auto& server : servers) {
            if (!isServerAvailable(server)) {
                logUnavailableServer(server, server);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

int main() {
    std::vector<std::string> servers = loadServers("servers.txt");
    monitorServers(servers);
    return 0;
}
