#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

void testNetworkLibraries() {
    // Test gethostbyname from netdb.h
    struct hostent *host = gethostbyname("www.google.com");
    if (host != nullptr) {
        std::cout << "netdb.h works! Host name: " << host->h_name << std::endl;
    } else {
        std::cerr << "netdb.h failed!" << std::endl;
    }

    // Test inet_ntop from arpa/inet.h
    char ipStr[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, host->h_addr_list[0], ipStr, sizeof(ipStr)) != nullptr) {
        std::cout << "arpa/inet.h works! IP address: " << ipStr << std::endl;
    } else {
        std::cerr << "arpa/inet.h failed!" << std::endl;
    }

    // Test usleep from unistd.h
    std::cout << "Testing unistd.h (usleep for 1 second)..." << std::endl;
    usleep(1000000); // Sleep for 1 second
    std::cout << "unistd.h works!" << std::endl;
}

int main() {
    // Test iostream
    std::cout << "iostream works!" << std::endl;

    // Test string
    std::string testString = "string works!";
    std::cout << testString << std::endl;

    // Test vector
    std::vector<int> testVector = {1, 2, 3};
    std::cout << "vector works! First element: " << testVector[0] << std::endl;

    // Test thread and mutex
    std::mutex mtx;
    std::thread testThread([&mtx]() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "thread and mutex work!" << std::endl;
    });
    testThread.join();

    // Test network libraries
    testNetworkLibraries();

    return 0;
}

