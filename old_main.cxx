/*
 
Thread Pool?
	Resource Management: Creating a thread for each port can exhaust system resources, especially when scanning large port ranges.
	Efficiency: Reusing a fixed number of threads reduces overhead from frequent thread creation and destruction.
	Control: Limits the number of concurrent threads, preventing potential performance degradation.

 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

std::mutex cout_mutex;
std::mutex queue_mutex;
std::condition_variable cv;
std::queue<int> port_queue;
bool done = false;

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

void worker(const std::string& ip) {
    while (true) {
        int port;

        // Acquire lock and check for available tasks
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [] { return !port_queue.empty() || done; });

            if (port_queue.empty()) {
                if (done) break;
                else continue;
            }

            port = port_queue.front();
            port_queue.pop();
        }

        // Scan the port outside the lock
        scanPort(ip, port);
    }
}

void scanPortsOnIP(const std::string& ip, int startPort, int endPort, int numThreads) {
    // Populate the task queue
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        for (int port = startPort; port <= endPort; ++port) {
            port_queue.push(port);
        }
    }

    // Set 'done' to true after all tasks are added
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        done = true;
    }
    cv.notify_all();

    // Create worker threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker, ip);
    }

    // Join threads
    for (auto& th : threads) {
        th.join();
    }
}

int main() {
    std::string ip;
    int startPort, endPort, numThreads;

    std::cout << "Enter IP address to scan: ";
    std::cin >> ip;

    std::cout << "Enter start port: ";
    std::cin >> startPort;

    std::cout << "Enter end port: ";
    std::cin >> endPort;

    std::cout << "Enter number of threads: ";
    std::cin >> numThreads;

    scanPortsOnIP(ip, startPort, endPort, numThreads);

    return 0;
}
