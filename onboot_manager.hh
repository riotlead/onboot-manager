#include <string>
#include <queue>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "cpu_monitor.hh"

#ifndef __ONBOOT_MANAGER_HH__
#define __ONBOOT_MANAGER_HH__

#include "openGA.hpp"

namespace amd {

namespace onboot {

class OnbootManager;

class Chromosome {
    public:
    std::vector<int> var;
    std::string to_string() const {
        std::string str("{");
        int i = 0;
        for (int delay : var) {
            str += ", var" + std::to_string(i) + ":" + std::to_string(delay);
        }
        str += "}";
        return str;
    }
};

class MiddleCost {
    public:
    double objective1;
};

typedef EA::Genetic<Chromosome,MiddleCost> GA_Type;
typedef EA::GenerationType<Chromosome,MiddleCost> Generation_Type;

class OnbootManager {
 public:
    class OnbootItem {
     public:
        OnbootItem(std::string appid, int priority, int delay) :
            appid_(appid), priority_(priority), delay_(delay) {}
        std::string appid_;
        int priority_;
        int delay_;
        int pid_;
        
        friend bool operator < (const OnbootItem& lhs, const OnbootItem& rhs) {
            return lhs.priority_ < rhs.priority_;
        }   
        friend bool operator > (const OnbootItem& lhs, const OnbootItem& rhs) {
            return lhs.priority_ > rhs.priority_;
        }   
             
    };
    class ILaunchInterface {
     public:
        virtual int Launch(std::string appid) = 0;
    };

    class IFinishCb {
     public:
        void finish();
    };


    OnbootManager(ILaunchInterface& launch);
    void AddList(std::string appid, int priority, int delay);
    void Start();
    void Adjust();
    void InitGenes(Chromosome& p, const std::function<double(void)> &rnd01);
    bool EvalSolution(const Chromosome& p, MiddleCost& c);
    Chromosome Mutate(const Chromosome& X_base, const std::function<double(void)> &rnd01, double shrink_scale);
    Chromosome Crossover(const Chromosome& X1, const Chromosome& X2, const std::function<double(void)> &rnd01);
    double SOFitness(const GA_Type::thisChromosomeType &X);
 private:
    OnbootManager();

 private:
    std::unordered_map<int, std::vector<int>> logs_;
    std::priority_queue<OnbootItem> queue_;
    std::vector<OnbootItem> list_;
    ILaunchInterface& launch_;
    CpuMonitor monitor_;

};

} // namespace onboot

} // namespace amd

#endif
