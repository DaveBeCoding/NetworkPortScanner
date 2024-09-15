#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

std::queue<int> dataQueue;
std::mutex mtx;
std::condition_variable cv;
bool finished = false;

void producer(int numItems) {
    for (int i = 1; i <= numItems; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            dataQueue.push(i);
            std::cout << "Produced: " << i << std::endl;
        }
        cv.notify_one(); // Notify the consumer that new data is available
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    }

    // Signal that production is finished
    {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
    }
    cv.notify_one(); // Notify the consumer to check the finished flag
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !dataQueue.empty() || finished; });

        while (!dataQueue.empty()) {
            int value = dataQueue.front();
            dataQueue.pop();
            std::cout << "Consumed: " << value << std::endl;
        }

        if (finished) {
            break;
        }
    }
    std::cout << "Consumer finished processing." << std::endl;
}

int main() {
    int numItems = 10; // Number of items to produce

    std::thread producerThread(producer, numItems);
    std::thread consumerThread(consumer);

    producerThread.join();
    consumerThread.join();

    std::cout << "Program completed successfully." << std::endl;

    return 0;
}

