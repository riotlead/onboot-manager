
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>

#include "onboot_manager.hh"
using namespace amd::onboot;

std::mutex EA::mtx_rand;

class TestDriver : public OnbootManager::ILaunchInterface {
    int Launch(std::string appid) override {
        int pid = fork();
        if (pid != 0) {
            return pid;
        }

        char* argv[1] = {strdup(appid.c_str())};

        execv("./test", argv);
        exit(EXIT_FAILURE);
        return 0;
    }
};

int main() {
    TestDriver driver;
    OnbootManager manager(driver);
    
    manager.AddList("ex1", 99, 0);
    manager.AddList("ex2", 88, 1800);
    manager.AddList("ex3", 77, 1800);
    manager.AddList("ex4", 66, 1800);
    manager.AddList("ex5", 55, 1800);
    manager.AddList("ex6", 44, 1800);
    manager.AddList("ex7", 33, 1800);
    manager.AddList("ex8", 22, 1800);
    manager.AddList("ex9", 22, 1800);
    manager.AddList("ex10", 22, 1800);

    manager.Start();

    return 0;
}
