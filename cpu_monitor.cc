//#include "core/onboot/cpu_monitor.hh"
#include "cpu_monitor.hh"
#include <iostream>
#include <stdio.h>
#include <fcntl.h>

namespace amd {

namespace onboot {

CpuMonitor::CpuMonitor() {

}

CpuMonitor::~CpuMonitor() {
  if (!disposed_) {
    Dispose();
  }
}

void CpuMonitor::Dispose() {
    thread_.join();
    disposed_ = true;
}

void _get_cpu_idle(unsigned long long *total, unsigned long long *idle)
{
    FILE *fp;
    int i;
    unsigned long long sum = 0;
    unsigned long long val; 
    unsigned long long iv = 0;
    char buf[4] = { 0, };

    fp = fopen("/proc/stat", "rt");

    if (fp == NULL)
        return;

    if (fscanf(fp, "%3s", buf) == -1) {
        fclose(fp);
        return;
    }

    for (i = 0; i < 10; i++) { 
        if (fscanf(fp, "%lld", &val) == -1) {
            fclose(fp);
            return;
        }

        if (sum + val < sum) {
            fclose(fp);
            return;
        }

        sum += val; 
        if (i == 3) /* idle */
            iv = val;
    }

    fclose(fp);

    *total = sum;
    *idle = iv;
}

void _get_pid_cpu(int pid, unsigned long long *usage) {
    FILE *fp;
    char path[255] = {0, };
    char buf[255];
    char state;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "rt");
    if (fp == NULL) {
        std::cout << "fopen error" << std::endl;
        return;
    } 

    int ival;
    if (fscanf(fp, "%d", &ival) == -1) {
        fclose(fp);
        
        return;   
    }

    if (fscanf(fp, "%s", &buf) == -1) {
        fclose(fp);        
        return;
    }
 
    if (fscanf(fp, "%s", &state) == -1) {
        fclose(fp);
        return;
    }

    int dummy; // for compiler happy
    unsigned long dummy2;
 
    if (fscanf(fp, "%d", &dummy) == -1) {
        fclose(fp);
        return;
    }; // ppid

    if (fscanf(fp, "%d", &dummy) == -1) {
        fclose(fp);
        return;
    }; // pgrp 9393 

    if (fscanf(fp, "%d", &dummy) == -1) {
        fclose(fp);
        return;
    };// sessionid 9393 

    if (fscanf(fp, "%d", &dummy) == -1) {
        fclose(fp);
        return;
    };//try_nr 1026
    
    if (fscanf(fp, "%d", &dummy) == -1) {
        fclose(fp);
        return;
    };//tpgid  9393 
    
    if (fscanf(fp, "%u", &dummy) == -1) {
        fclose(fp);
        return;
    };//flags 4194560 
    
    if (fscanf(fp, "%lu", &dummy2) == -1) {
        fclose(fp);
        return;
    };//minflt 264789 

    if (fscanf(fp, "%lu", &dummy2) == -1) {
        fclose(fp);
        return;
    };//cminflt 1897

    if (fscanf(fp, "%lu", &dummy2) == -1) {
        fclose(fp);
        return;
    };//majflt
    
    if (fscanf(fp, "%lu", &dummy2) == -1) {
        fclose(fp);
        return;
    };//cmajflt
    
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long cutime;
    unsigned long long cstime;

    if (fscanf(fp, "%llu", &utime) == -1) {
        fclose(fp);
        return;
    };
    if (fscanf(fp, "%llu", &stime) == -1) {
        fclose(fp);
        return;
    };
    if (fscanf(fp, "%llu", &cutime) == -1) {
        fclose(fp);
        return;
    };
    if (fscanf(fp, "%llu", &cstime) == -1) {
        fclose(fp);
        return;
    };
    fclose(fp);

    *usage = utime + stime + cutime + cstime;
}

void CpuMonitor::Start() {
  thread_ = std::thread([&]() {
    auto start = std::chrono::steady_clock::now();
    int i = 0;
    do {
        auto now = std::chrono::steady_clock::now();
          if (now - start > std::chrono::seconds(18)) {
              break;
        }
        
        unsigned long long total;
        unsigned long long idle;

        _get_cpu_idle(&total, &idle);
        unsigned int base = static_cast<unsigned int>(total - idle);
        history_.emplace_back(i, base, now);
        ClockFrame& frame = history_.back();
        {
            std::lock_guard<std::recursive_mutex> lock(GetMutex());
            for (auto& pid : pids_) {
                unsigned long long usage;
                _get_pid_cpu(pid.first, &usage);
                frame.Log(pid.first, usage);
                pid.second--;                
            }
            pids_.remove_if([](auto& pid) {
                return pid.second < 1;
            });
        }
        
        i++;
        auto period = std::chrono::milliseconds(100) - (std::chrono::steady_clock::now() - now);
        std::this_thread::sleep_for(period);
    } while (true);

  });
  disposed_ = false;
}

void CpuMonitor::Dump() {
    for (auto& frame : history_) {
        std::cout << frame.frame_ << ":" << frame.base_ << std::endl;
        for (auto& log : frame.log_) {
            std::cout << log.first << ":" << log.second << " ";
        }
        std::cout << std::endl;
    }
}

std::vector<ClockFrame>& CpuMonitor::GetResult() {
    return history_;
}

void CpuMonitor::Add(int pid) {
    std::lock_guard<std::recursive_mutex> lock(GetMutex());
    pids_.emplace_back(pid, 30);
}

std::recursive_mutex& CpuMonitor::GetMutex() const {
  return mutex_;
}

} // namespace onboot

} // namespace amd