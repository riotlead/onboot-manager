#include <thread>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>

int main(int argc, char* argv[]) {
    // if (argc > 1) {
    //     std::cout << argv[1] << std::endl;
    // }

    auto start = std::chrono::steady_clock::now();
    int delay = 1500 + random() % 1500;

    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(delay)) {
        int i = 0;
        for (i = 0; i < 999999; i++) {
            i++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds() % 100);
    }
}