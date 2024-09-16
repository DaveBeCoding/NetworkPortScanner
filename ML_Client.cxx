#include <iostream>
#include <cstdlib>  // For system()

// Function to call the Python script for machine learning analysis
void run_ml_analysis() {
    std::cout << "Running Machine Learning Analysis on Port Data...\n";
    int result = system("python3 ml_analysis.py");
    
    // Check if the Python script ran successfully
    if (result == 0) {
        std::cout << "Machine Learning Analysis Complete.\n";
    } else {
        std::cerr << "Error running Python script. Make sure Python is installed and configured correctly.\n";
    }
}

int main() {
    // Run the machine learning analysis
    run_ml_analysis();
    return 0;
}