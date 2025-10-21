#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
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

public:
  Task(int pid, size_t arrival_time, size_t burst_time, int priority) {
    m_pid = pid;
    m_arrival_time = arrival_time;
    m_burst_time = burst_time;
    m_priority = priority;
    m_cpu_time = 0;
  }

  bool is_ready(size_t total_time_elapsed) const {
    return total_time_elapsed >= m_arrival_time;
  }

  void tick() {
    m_cpu_time += 1;
    std::printf("pid: %d, time_on_cpu: %zu, time_left: %zu \n", pid(),
                time_on_cpu(), time_left());
  }

  int burst_time() const { return m_burst_time; }

  int pid() const { return m_pid; }

  size_t time_left() const { return m_burst_time - m_cpu_time; }

  int priority() const { return m_priority; }

  size_t time_on_cpu() const { return m_cpu_time; }

  size_t arrival_time() const { return m_arrival_time; }
};
class Scheduler {

protected:
  size_t m_time_elapsed;
  size_t m_idle_time;
  std::vector<Task> m_task_data;
  std::vector<size_t> m_uninitialized_tasks;
  std::vector<size_t> m_ready_tasks;
  std::unordered_map<int, size_t> m_task_start_times;
  std::unordered_map<int, size_t> m_turnaround_times;

public:
  // Line format: Pid Arrival_Time Burst_Time Priority
  void load_tasks(std::string tasks_string) {
    std::istringstream string_stream(tasks_string);
    std::string line;
    size_t i = 0;
    while (std::getline(string_stream, line)) {
      if (line.empty()) {
        continue;
      }

      std::istringstream line_stream(line);
      int pid, priority;
      size_t arrival_time, burst_time;

      line_stream >> pid;
      line_stream >> arrival_time;
      line_stream >> burst_time;
      line_stream >> priority;
      m_task_data.push_back(Task(pid, arrival_time, burst_time, priority));
      m_uninitialized_tasks.push_back(i);
      i++;
    }
  }

  void print_results() const {
    /*
    Average waiting time
    Average response time
    Average turnaround time
    CPU utilization rate
    */

    float num_tasks = static_cast<float>(m_task_data.size());
    float total_wait_time = 0.0;
    float total_turnaround_time = 0.0;
    float total_response_time = 0.0;
    for (auto task : m_task_data) {
      size_t first_cycle = m_task_start_times.at(task.pid());
      size_t last_cycle = m_turnaround_times.at(task.pid());

      // std::printf("first cycle: %zu\n last_cycle: %zu\n,arrival_time: %zu\n",
      //             first_cycle, last_cycle, task.arrival_time());
      float response_time =
          static_cast<float>(first_cycle - task.arrival_time());
      float turnaround_time =
          static_cast<float>(last_cycle - task.arrival_time());
      float wait_time = turnaround_time - static_cast<float>(task.burst_time());
      total_wait_time += wait_time;
      total_turnaround_time += turnaround_time;
      total_response_time += response_time;
    }
    float avg_wait_time = total_wait_time / num_tasks;
    float avg_response_time = total_response_time / num_tasks;
    float avg_turnaround_time = total_turnaround_time / num_tasks;
    float cpu_utilization = 1.0 - (static_cast<float>(m_idle_time) /
                                   static_cast<float>(m_time_elapsed));

    std::printf("Avg Wait Time: %f\nAvg Response Time: %f\n Avg Turnaround "
                "Time: %f\nCPU Utilization:%f\n",
                avg_wait_time, avg_response_time, avg_turnaround_time,
                cpu_utilization);
  }
};

// First come first serve
class FCFS : public Scheduler {

  void update_ready_list() {

    auto comp = [this](size_t a, size_t b) {
      auto &task_a = m_task_data[a];
      auto &task_b = m_task_data[b];

      if (task_a.arrival_time() != task_b.arrival_time()) {
        return task_a.arrival_time() < task_b.arrival_time();
      }
      return task_a.priority() < task_b.priority();
    };

    if (!m_ready_tasks.empty()) {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_left() == 0) {
        m_turnaround_times.insert({current_task.pid(), m_time_elapsed});
        m_ready_tasks.erase(m_ready_tasks.begin());
      }
    }

    std::vector<size_t> tasks_to_add = {};

    auto new_end = std::remove_if(m_uninitialized_tasks.begin(),
                                  m_uninitialized_tasks.end(),
                                  [this, &tasks_to_add](size_t task_index) {
                                    auto &task = m_task_data[task_index];
                                    if (task.is_ready(m_time_elapsed)) {
                                      tasks_to_add.push_back(task_index);
                                      return true;
                                    }
                                    return false;
                                  });

    m_uninitialized_tasks.erase(new_end, m_uninitialized_tasks.end());
    std::sort(tasks_to_add.begin(), tasks_to_add.end(), comp);

    for (size_t task_index : tasks_to_add) {
      m_ready_tasks.push_back(task_index);
    }
  }

  bool can_make_scheduling_decision() {
    if (!m_ready_tasks.empty()) {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      return current_task.time_left() == 0;
    } else {
      return true;
    }
  }

  bool all_tasks_completed() {
    return m_uninitialized_tasks.empty() && m_ready_tasks.empty();
  }

public:
  bool tick() {
    std::printf("Tick: %zu ", m_time_elapsed);
    if (all_tasks_completed()) {
      return false;
    }

    if (can_make_scheduling_decision()) {
      update_ready_list();
    }

    if (m_ready_tasks.empty()) {
      m_idle_time += 1;
    } else {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_on_cpu() == 0) {
        m_task_start_times.insert({current_task.pid(), m_time_elapsed});
      }
      current_task.tick();
    }
    m_time_elapsed += 1;
    return true;
  }
};

// Shortest job first
class SJF : public Scheduler {

  void update_ready_list() {

    auto comp = [this](size_t a, size_t b) {
      auto &task_a = m_task_data[a];
      auto &task_b = m_task_data[b];

      if (task_a.burst_time() != task_b.burst_time()) {

        return task_a.burst_time() < task_b.burst_time();
      }
      return task_a.priority() < task_b.priority();
    };

    if (!m_ready_tasks.empty()) {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_left() == 0) {
        m_turnaround_times.insert({current_task.pid(), m_time_elapsed});
        m_ready_tasks.erase(m_ready_tasks.begin());
      }
    }

    std::vector<size_t> tasks_to_add = {};

    auto new_end = std::remove_if(m_uninitialized_tasks.begin(),
                                  m_uninitialized_tasks.end(),
                                  [this, &tasks_to_add](size_t task_index) {
                                    auto &task = m_task_data[task_index];
                                    if (task.is_ready(m_time_elapsed)) {
                                      tasks_to_add.push_back(task_index);
                                      return true;
                                    }
                                    return false;
                                  });

    m_uninitialized_tasks.erase(new_end, m_uninitialized_tasks.end());
    std::sort(tasks_to_add.begin(), tasks_to_add.end(), comp);

    for (size_t task_index : tasks_to_add) {
      m_ready_tasks.push_back(task_index);
    }
  }

  bool can_make_scheduling_decision() {
    if (!m_ready_tasks.empty()) {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      return current_task.time_left() == 0;
    } else {
      return true;
    }
  }

  bool all_tasks_completed() {
    return m_uninitialized_tasks.empty() && m_ready_tasks.empty();
  }

public:
  bool tick() {
    std::printf("Tick: %zu ", m_time_elapsed);
    if (all_tasks_completed()) {
      return false;
    }

    if (can_make_scheduling_decision()) {
      update_ready_list();
    }

    if (m_ready_tasks.empty()) {
      m_idle_time += 1;
    } else {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_on_cpu() == 0) {
        m_task_start_times.insert({current_task.pid(), m_time_elapsed});
      }
      current_task.tick();
    }
    m_time_elapsed += 1;
    return true;
  }
};

// Preemptive priority scheduling
class PPS : public Scheduler {

  void update_ready_list() {

    auto comp = [this](size_t a, size_t b) {
      auto &task_a = m_task_data[a];
      auto &task_b = m_task_data[b];

      if (task_a.priority() != task_b.priority()) {

        return task_a.priority() < task_b.priority();
      }
      return task_a.burst_time() < task_b.burst_time();
    };

    if (!m_ready_tasks.empty()) {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_left() == 0) {
        m_turnaround_times.insert({current_task.pid(), m_time_elapsed});
        m_ready_tasks.erase(m_ready_tasks.begin());
      }
    }

    std::vector<size_t> tasks_to_add = {};

    auto new_end = std::remove_if(m_uninitialized_tasks.begin(),
                                  m_uninitialized_tasks.end(),
                                  [this, &tasks_to_add](size_t task_index) {
                                    auto &task = m_task_data[task_index];
                                    if (task.is_ready(m_time_elapsed)) {
                                      tasks_to_add.push_back(task_index);
                                      return true;
                                    }
                                    return false;
                                  });

    m_uninitialized_tasks.erase(new_end, m_uninitialized_tasks.end());

    for (size_t task_index : tasks_to_add) {
      m_ready_tasks.push_back(task_index);
    }

    std::sort(m_ready_tasks.begin(), m_ready_tasks.end(), comp);
  }

  bool can_make_scheduling_decision() {
    if (!m_ready_tasks.empty()) {

      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_left() == 0) {
        return true;
      }
      for (size_t task_index : m_uninitialized_tasks) {
        Task &task = m_task_data[task_index];
        if (task.priority() < current_task.priority()) {
          return true;
        }
      }
      return false;
    } else {
      return true;
    }
  }

  bool all_tasks_completed() {
    return m_uninitialized_tasks.empty() && m_ready_tasks.empty();
  }

public:
  bool tick() {
    std::printf("Tick: %zu ", m_time_elapsed);
    if (all_tasks_completed()) {
      return false;
    }

    if (can_make_scheduling_decision()) {
      update_ready_list();
    }

    if (m_ready_tasks.empty()) {
      m_idle_time += 1;
    } else {
      Task &current_task = m_task_data[m_ready_tasks.front()];
      if (current_task.time_on_cpu() == 0) {
        m_task_start_times.insert({current_task.pid(), m_time_elapsed});
      }
      current_task.tick();
    }
    m_time_elapsed += 1;
    return true;
  }
};

// Round Robin
class RR : public Scheduler {
  size_t quantum;
};

int main() {
  auto input_file = std::ifstream("../input.txt");
  std::string input_data =
      std::string(std::istreambuf_iterator<char>(input_file),
                  std::istreambuf_iterator<char>());
  std::printf("input_data: \n%s", input_data.c_str());

  auto fcfs = FCFS();
  fcfs.load_tasks(input_data);
  while (fcfs.tick()) {
  };

  fcfs.print_results();

  auto sjf = SJF();
  sjf.load_tasks(input_data);
  while (sjf.tick()) {
  };

  sjf.print_results();

  auto pps = PPS();
  pps.load_tasks(input_data);

  while (pps.tick()) {
  };
  pps.print_results();

  return 0;
}
