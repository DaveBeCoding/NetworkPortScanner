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
#include <fcntl.h>       // For fcntl()
#include <sys/select.h>  // For select()
#include <errno.h>
#include <sqlite3.h>
#include <ctime>

std::mutex cout_mutex;
std::mutex queue_mutex;
std::condition_variable cv;
std::queue<int> port_queue;
bool done = false;

void scanPort(const std::string& ip, int port);
void worker(const std::string& ip);
void scanPortsOnIP(const std::string& ip, int startPort, int endPort, int numThreads);
void store_scan_data(int port, const std::string& banner, const std::string& ip_address);

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


// void scanPort(const std::string& ip, int port) {
//     int sockfd;
//     sockaddr_in addr;

//     // Create socket
//     sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd < 0) return; // Socket creation failed

//     // Set up address structure
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
//     inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

//     // Set socket to non-blocking
//     fcntl(sockfd, F_SETFL, O_NONBLOCK);

//     // Try to connect
//     int result = connect(sockfd, (sockaddr*)&addr, sizeof(addr));

//     // Check if connection is in progress or already connected
//     if (result < 0 && errno != EINPROGRESS) {
//         // Connection failed immediately
//         close(sockfd);
//         return;
//     }

//     // Use select to wait for the socket to become writable (connected) or for an error
//     fd_set wfds;
//     struct timeval tv;
//     FD_ZERO(&wfds);
//     FD_SET(sockfd, &wfds);
//     tv.tv_sec = 1;  // 1-second timeout
//     tv.tv_usec = 0;

//     result = select(sockfd + 1, NULL, &wfds, NULL, &tv);
//     if (result > 0 && FD_ISSET(sockfd, &wfds)) {
//         int error = 0;
//         socklen_t len = sizeof(error);

//         // Check for socket errors
//         if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
//             // Error occurred while getting socket options
//             close(sockfd);
//             return;
//         }

//         if (error == 0) {
//             // Connection successful
//             std::string output = "Port " + std::to_string(port) + " is open on " + ip;

//             // Now, set up select to wait for data to be readable
//             fd_set rfds;
//             FD_ZERO(&rfds);
//             FD_SET(sockfd, &rfds);
//             tv.tv_sec = 1;  // 1-second timeout
//             tv.tv_usec = 0;

//             result = select(sockfd + 1, &rfds, NULL, NULL, &tv);
//             if (result > 0 && FD_ISSET(sockfd, &rfds)) {
//                 // Data is available to read
//                 char buffer[1024];
//                 ssize_t bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
//                 if (bytes > 0) {
//                     buffer[bytes] = '\0';
//                     output += " | Banner: " + std::string(buffer);
//                 }
//             }

//             // Thread-safe output
//             {
//                 std::lock_guard<std::mutex> guard(cout_mutex);
//                 std::cout << output << std::endl;
//             }
//         }
//         // No need for else block; if error != 0, connection failed
//     }

//     // Close the socket
//     close(sockfd);
// }

// Function to insert scan data into SQLite database
void store_scan_data(int port, const std::string& banner, const std::string& ip_address) {
    sqlite3 *db;
    char *err_msg = 0;
    
    // Open or create the database
    int rc = sqlite3_open("network_scanner.db", &db);
    
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }
    
    // Create table if it doesn't exist
    const char* create_table_sql = 
        "CREATE TABLE IF NOT EXISTS port_scans ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "port INTEGER,"
        "banner TEXT,"
        "ip_address TEXT,"
        "timestamp TEXT,"
        "day TEXT"
        ");";
    
    rc = sqlite3_exec(db, create_table_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }
    
    // Get the current time
    std::time_t now = std::time(nullptr);
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    
    char day[10];
    std::strftime(day, sizeof(day), "%A", std::localtime(&now));
    
    // Prepare the insert statement
    std::string sql = "INSERT INTO port_scans (port, banner, ip_address, timestamp, day) VALUES (" +
                      std::to_string(port) + ", '" + banner + "', '" + ip_address + "', '" + timestamp + "', '" + day + "');";
    
    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
}

void scanPort(const std::string& ip, int port) {
    int sockfd;
    sockaddr_in addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return; // Socket creation failed

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    // Set socket to non-blocking
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // Try to connect
    int result = connect(sockfd, (sockaddr*)&addr, sizeof(addr));

    // Check if connection is in progress or already connected
    if (result < 0 && errno != EINPROGRESS) {
        // Connection failed immediately
        close(sockfd);
        return;
    }

    // Use select to wait for the socket to become writable (connected) or for an error
    fd_set wfds;
    struct timeval tv;
    FD_ZERO(&wfds);
    FD_SET(sockfd, &wfds);
    tv.tv_sec = 1;  // 1-second timeout
    tv.tv_usec = 0;

    result = select(sockfd + 1, NULL, &wfds, NULL, &tv);
    if (result > 0 && FD_ISSET(sockfd, &wfds)) {
        int error = 0;
        socklen_t len = sizeof(error);

        // Check for socket errors
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            close(sockfd);
            return;
        }

        if (error == 0) {
            // Connection successful
            std::string output = "Port " + std::to_string(port) + " is open on " + ip;
            std::string banner = "";

            // Now, set up select to wait for data to be readable
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sockfd, &rfds);
            tv.tv_sec = 1;  // 1-second timeout
            tv.tv_usec = 0;

            result = select(sockfd + 1, &rfds, NULL, NULL, &tv);
            if (result > 0 && FD_ISSET(sockfd, &rfds)) {
                // Data is available to read
                char buffer[1024];
                ssize_t bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    banner = buffer;
                    output += " | Banner: " + banner;
                }
            }

            // Thread-safe output
            {
                std::lock_guard<std::mutex> guard(cout_mutex);
                std::cout << output << std::endl;
            }

            // Store the scan data in the SQLite database
            store_scan_data(port, banner, ip);
        }
    }

    close(sockfd);
}


void scanPortsOnIP(const std::string& ip, int startPort, int endPort, int numThreads) {
    // Populate the task queue and set 'done' to true
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        for (int port = startPort; port <= endPort; ++port) {
            port_queue.push(port);
        }
        done = true;
    }

    // Notify workers that tasks are available
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

