#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <list>

#ifndef __CPU_MONITOR_HH__
#define __CPU_MONITOR_HH__

namespace amd {

namespace onboot {

class ClockFrame {
 public:
  int frame_;
  int base_;
  std::chrono::steady_clock::time_point time_;
  std::vector<std::pair<int, int>> log_;
  ClockFrame(int frame, int base, std::chrono::steady_clock::time_point time) : 
    frame_(frame), base_(base), time_(time) {}
  void Log(int pid, int usage) {
      log_.emplace_back(pid, usage);
  }
};

class CpuMonitor {
 public:
  CpuMonitor();
  ~CpuMonitor();
  void Dispose();
  void Start();
  void Add(int pid);
  void Dump();
  std::vector<ClockFrame>& GetResult();

 private:
  std::recursive_mutex& GetMutex() const;

 private:
  std::vector<ClockFrame> history_;
  std::list<std::pair<int, int>> pids_;
  bool disposed_ = true;
  std::thread thread_;
  mutable std::recursive_mutex mutex_;
};

} // namespace onboot

} // namespace amd

#endif
