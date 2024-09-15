#include <iostream>
#include <string>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>

int main() {
    struct ifaddrs *interfaces = nullptr;
    struct ifaddrs *temp_addr = nullptr;

    // Retrieve the current interfaces. getifaddrs() returns 0 on success.
    if (getifaddrs(&interfaces) == 0) {
        temp_addr = interfaces;

        while (temp_addr != nullptr) {
            if (temp_addr->ifa_addr != nullptr) {
                // Check if it is IPv4 address
                if (temp_addr->ifa_addr->sa_family == AF_INET) {
                    // Exclude loopback interface
                    if (!(temp_addr->ifa_flags & IFF_LOOPBACK)) {
                        char ipAddress[INET_ADDRSTRLEN];
                        void *addrPtr = &((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr;

                        inet_ntop(AF_INET, addrPtr, ipAddress, INET_ADDRSTRLEN);

                        std::cout << "Interface: " << temp_addr->ifa_name
                                  << " | IP Address: " << ipAddress << std::endl;
                    }
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    } else {
        std::cerr << "Error getting interfaces." << std::endl;
        return 1;
    }

    // Free the memory allocated by getifaddrs()
    freeifaddrs(interfaces);

    return 0;
}

