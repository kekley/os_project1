
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
class Task {
  int m_pid;
  size_t m_arrival_time;
  size_t m_burst_time;
  size_t m_cpu_time;
  int m_priority;
  size_t m_first_cycle;

public:
  Task(int pid, size_t arrival_time, size_t burst_time, int priority) {
    m_pid = pid;
    m_arrival_time = arrival_time;
    m_burst_time = burst_time;
    m_priority = priority;
    m_cpu_time = 0;
    m_first_cycle = SIZE_MAX;
  }

  bool is_ready(size_t total_time_elapsed) {
    return total_time_elapsed >= m_arrival_time;
  }

  bool tick() {
    m_cpu_time += 1;
    return m_burst_time == m_cpu_time;
  }

  int pid() { return m_pid; }

  size_t time_left() { return m_burst_time - m_cpu_time; }

  int priority() { return m_priority; }

  size_t time_elapsed() { return m_cpu_time; }
};
class Scheduler {

protected:
  size_t total_time_elapsed;
  std::vector<Task> task_data;
  std::vector<size_t> unintialized_tasks;
  std::vector<size_t> ready_tasks;
  std::vector<size_t> completed_tasks;
  std::unordered_map<int, size_t> first_scheduling;

public:
  // Line format: Pid Arrival_Time Burst_Time Priority
  void load_tasks(std::string tasks_string) {
    std::istringstream string_stream(tasks_string);
    std::string line;
    size_t i = 0;
    while (std::getline(string_stream, line)) {
      int pid, priority;
      size_t arrival_time, burst_time;

      string_stream >> pid;
      string_stream >> arrival_time;
      string_stream >> burst_time;
      string_stream >> priority;
      task_data.push_back(Task(pid, arrival_time, burst_time, priority));
      unintialized_tasks.push_back(i);
      i++;
    }
  }
  void print_status() const {
    for (auto task : task_data) {
      std::printf("pid: %d, time_on_cpu: %zu, time_left: %zu", task.pid(),
                  task.time_elapsed(), task.time_left());
    }
  }
};

// First come first serve
class FCFS : Scheduler {
  void update_ready_list() {
    auto comp = [this](size_t a, size_t b) {
      auto &task_a = task_data[a];
      auto &task_b = task_data[b];

      return task_a.priority() > task_b.priority();
    };

    std::vector<size_t> tasks_to_add = {};
    for (size_t task_index : unintialized_tasks) {
      auto &task = task_data[task_index];
      if (task.is_ready(total_time_elapsed)) {
        tasks_to_add.push_back(task_index);
      }
    }
    std::sort(tasks_to_add.begin(), tasks_to_add.end(), comp);
  }

  bool tick() { update_ready_list(); }
};

// Shortest job first
class SJF : Scheduler {};

// Preemptive priority scheduling
class PPS : Scheduler {};

// Round Robin
class RR : Scheduler {
  size_t quantum;
};

int main() { return 0; }
