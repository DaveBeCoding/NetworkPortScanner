#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

std::mutex cout_mutex;

void scanPort(const std::string& ip, int port) {
    int sockfd;
    sockaddr_in addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return; // Socket creation failed

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    // Try to connect
    int result = connect(sockfd, (sockaddr*)&addr, sizeof(addr));

    if (result == 0) {
        std::lock_guard<std::mutex> guard(cout_mutex);
        std::cout << "Port " << port << " is open on " << ip << std::endl;
    }

    close(sockfd);
}

void scanPortsOnIP(const std::string& ip, int startPort, int endPort) {
    std::vector<std::thread> threads;

    for (int port = startPort; port <= endPort; ++port) {
        threads.emplace_back(scanPort, ip, port);
    }

    for (auto& th : threads) {
        th.join();
    }
}

int main() {
    std::string ip;
    int startPort, endPort;

    std::cout << "Enter IP address to scan: ";
    std::cin >> ip;

    std::cout << "Enter start port: ";
    std::cin >> startPort;

    std::cout << "Enter end port: ";
    std::cin >> endPort;

    scanPortsOnIP(ip, startPort, endPort);

    return 0;
}
