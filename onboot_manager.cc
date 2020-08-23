#include "cpu_monitor.hh"
#include "onboot_manager.hh"
#include "openGA.hpp"
#include <iostream>
namespace amd {

namespace onboot {

typedef EA::Genetic<OnbootManager::Chromosome,OnbootManager::MiddleCost> GA_Type;
typedef EA::GenerationType<OnbootManager::Chromosome,OnbootManager::MiddleCost> Generation_Type;


OnbootManager::OnbootManager(ILaunchInterface& launch) : launch_(launch) {
    
}

void OnbootManager::InitGenes(Chromosome& p, const std::function<double(void)> &rnd01) {
    // rnd01() gives a random number in 0~1
    int delay = 0;
    logs_.emplace(0, std::vector<int>()); // base
    for (auto& item : list_) {
        p.var.push_back(delay);
        delay += item.delay_ / 100 + (180 / list_.size()) * rnd01();
        logs_.emplace(item.pid_, std::vector<int>());
    }

    std::vector<ClockFrame>& result = monitor_.GetResult();
    int i = 0;
    int N = result.size();
    for (i = 0; i < N; i++) {
        ClockFrame& frame = result[i];
        int cpus = 0;
        for (auto& pid_usage : frame.log_) {
            cpus += pid_usage.second;       
            logs_.at(pid_usage.first).push_back(pid_usage.second);
        }
        logs_.at(0).push_back(frame.base_ - cpus);
    }
}

bool OnbootManager::EvalSolution(const Chromosome& p, MiddleCost& c) {
    float sum = 0.0f;
    int i = 0;
    int N = logs_.at(0).size();
    for (i = 0; i < N; i++) {
        int t = logs_.at(0).at(i);
        int j = 0;
        for (j = 0; j < p.var.size(); j++) {
            int start_pos = p.var[j];
            int end_pos = start_pos + logs_.at(list_[j].pid_).size();
            if (start_pos > i && i < end_pos) {
                t += logs_.at(list_[j].pid_).at(i - start_pos);
            }
        }
        sum = sum + (1.0f / (float)t);
    }
    c.objective1 = ((float)N / sum);
    return true;
}

OnbootManager::Chromosome OnbootManager::Mutate(const Chromosome& X_base, const std::function<double(void)> &rnd01, double shrink_scale) {
    Chromosome X_new;
    const double mu = 0.2*shrink_scale; // mutation radius (adjustable)
    bool in_range;
    do{
        in_range=true;
        X_new=X_base;
        for (auto& i : X_new.var) {
            i += mu*(rnd01()-rnd01());
            in_range=in_range&&(i>=0 && i<180);
        }        
    } while(!in_range);
    return X_new;
}

OnbootManager::Chromosome OnbootManager::Crossover(const Chromosome& X1, const Chromosome& X2, const std::function<double(void)> &rnd01) {
    Chromosome X_new;
    double r;
    r=rnd01();
    
    int i = 0;
    for (i = 0; i < X1.var.size(); i++) {
        X_new.var.push_back(r*X1.var[i]+(1.0-r)*X2.var[i]);
        r=rnd01();
    }
    return X_new;
}

double OnbootManager::SOFitness(const GA_Type::thisChromosomeType &X){
    // finalize the cost
    double final_cost=0.0;
    final_cost+=X.middle_costs.objective1;
    return final_cost;
}

void OnbootManager::AddList(std::string appid, int priority, int delay) {
    queue_.emplace(appid, priority, delay);
}

void OnbootManager::Start() {
    monitor_.Start();

    while (!queue_.empty()) {
        OnbootItem item = queue_.top();
        queue_.pop();
        
        int pid = launch_.Launch(item.appid_);
        monitor_.Add(pid);
        item.pid_ = pid;

        list_.push_back(item);
        
        OnbootItem nextItem = queue_.top();
        std::this_thread::sleep_for(std::chrono::milliseconds(nextItem.delay_));
    }

    //monitor_.Dump();
}

void OnbootManager::Adjust() {
    monitor_.Dispose();

    EA::Chronometer timer;
    timer.tic();
    using amd::onboot::OnbootManager;
    GA_Type ga_obj;
    ga_obj.problem_mode=EA::GA_MODE::SOGA;
    ga_obj.multi_threading=true;
    ga_obj.idle_delay_us=1; // switch between threads quickly
    ga_obj.dynamic_threading=false;
    ga_obj.verbose=false;
    ga_obj.population=200;
    ga_obj.generation_max=1000;
    ga_obj.calculate_SO_total_fitness=SOFitness;
    ga_obj.init_genes=InitGenes;
    ga_obj.eval_solution=EvalSolution;
    ga_obj.mutate=Mutate;
    ga_obj.crossover=Crossover;
    ga_obj.crossover_fraction=0.7;
    ga_obj.mutation_rate=0.2;
    ga_obj.best_stall_max=10;
    ga_obj.elite_count=10;
    ga_obj.solve();

    cout<<"The problem is optimized in "<<timer.toc()<<" seconds."<<endl;
}

} // namespace onboot

} // namespace amd