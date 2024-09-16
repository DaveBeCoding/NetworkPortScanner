#include <iostream>
#include <sqlite3.h>

// Function to calculate basic statistics on the scanned port data
void run_port_statistics() {
    sqlite3* db;
    sqlite3_stmt* stmt;
    const char* query;
    int rc;

    // Open the SQLite database
    rc = sqlite3_open("network_scanner.db", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Query 1: Find the most frequently open ports
    query = "SELECT port, COUNT(port) as occurrences FROM port_scans GROUP BY port ORDER BY occurrences DESC LIMIT 5;";
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        std::cout << "Top 5 Most Frequently Open Ports:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int port = sqlite3_column_int(stmt, 0);
            int occurrences = sqlite3_column_int(stmt, 1);
            std::cout << "Port: " << port << " | Occurrences: " << occurrences << "\n";
        }
        sqlite3_finalize(stmt);
    }

    // Query 2: Find the most common banner
    query = "SELECT banner, COUNT(banner) as occurrences FROM port_scans WHERE banner IS NOT NULL GROUP BY banner ORDER BY occurrences DESC LIMIT 5;";
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        std::cout << "\nTop 5 Most Common Banners:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* banner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int occurrences = sqlite3_column_int(stmt, 1);
            std::cout << "Banner: " << banner << " | Occurrences: " << occurrences << "\n";
        }
        sqlite3_finalize(stmt);
    }

    // Query 3: Find the day when most ports were open
    query = "SELECT day, COUNT(day) as occurrences FROM port_scans GROUP BY day ORDER BY occurrences DESC LIMIT 1;";
    rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);
    if (rc == SQLITE_OK) {
        std::cout << "\nDay With Most Open Ports:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* day = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            int occurrences = sqlite3_column_int(stmt, 1);
            std::cout << "Day: " << day << " | Open Ports: " << occurrences << "\n";
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
}

int main() {
    // Run the statistics function
    run_port_statistics();
    return 0;
}
