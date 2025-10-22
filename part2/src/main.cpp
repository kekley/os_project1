#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Object that stores all the task data read in from the file and how long this
// task has been run
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

  // Has the arrival time been reached?
  bool is_ready(size_t total_time_elapsed) const {
    return total_time_elapsed >= m_arrival_time;
  }

  // Increment cpu time and print status
  void tick() {
    m_cpu_time += 1;
    std::cout << "pid: " << pid() << " time on cpu: " << time_on_cpu()
              << " time left: " << time_left() << std::endl;
  }

  // Getters
  int burst_time() const { return m_burst_time; }

  int pid() const { return m_pid; }

  size_t time_left() const { return m_burst_time - m_cpu_time; }

  int priority() const { return m_priority; }

  size_t time_on_cpu() const { return m_cpu_time; }

  size_t arrival_time() const { return m_arrival_time; }
};

// Base class for all our schedulers
class Scheduler {
protected:
  size_t m_time_elapsed;
  size_t m_idle_time;
  std::vector<Task> m_task_data;
  std::vector<size_t> m_not_arrived_task_indices;
  std::vector<size_t> m_ready_task_indices;
  std::unordered_map<int, size_t> m_task_start_times;
  std::unordered_map<int, size_t> m_turnaround_times;

  // Checks if the task at the front of the ready queue is finished. If so,
  // records its turnaround time and removes it
  // Returns true if a task was finished and removed, false otherwise.
  bool handle_any_finished_task() {
    if (!m_ready_task_indices.empty()) {
      Task &current_task = m_task_data[m_ready_task_indices.front()];
      if (current_task.time_left() == 0) {
        m_turnaround_times.insert({current_task.pid(), m_time_elapsed});
        m_ready_task_indices.erase(m_ready_task_indices.begin());
        return true;
      }
    }
    return false;
  }

  // Finds all tasks that have arrived by this point and moves them into a new
  // vector. Returns a vector of indices for newly arrived tasks.
  std::vector<size_t> get_newly_arrived_tasks() {
    std::vector<size_t> tasks_to_add = {};

    auto new_end = std::remove_if(m_not_arrived_task_indices.begin(),
                                  m_not_arrived_task_indices.end(),
                                  [this, &tasks_to_add](size_t task_index) {
                                    auto &task = m_task_data[task_index];
                                    if (task.is_ready(m_time_elapsed)) {
                                      tasks_to_add.push_back(task_index);
                                      return true;
                                    }
                                    return false;
                                  });

    m_not_arrived_task_indices.erase(new_end, m_not_arrived_task_indices.end());
    return tasks_to_add;
  }

  // Methods that will be specialized for each scheduler
  virtual void on_task_tick(Task &_) {
    // Suppress unused warning
    (void)_;
  }

  virtual void update_ready_list() = 0;

  virtual bool can_make_scheduling_decision() = 0;

public:
  Scheduler() : m_time_elapsed(0), m_idle_time(0) {}
  virtual ~Scheduler() {}

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
      m_not_arrived_task_indices.push_back(i);
      i++;
    }
  }

  bool all_tasks_completed() const {
    return m_not_arrived_task_indices.empty() && m_ready_task_indices.empty();
  }

  // The function that drives the scheduler, stepping one time unit every time
  // it's called. returns true if there are unfinished tasks and false if all
  // tasks have been completed

  bool tick() {
    if (can_make_scheduling_decision()) {
      update_ready_list();
    }

    // Are we done?
    if (all_tasks_completed()) {
      return false;
    }

    // Print what tick we're on
    std::cout << "Tick: " << m_time_elapsed << " ";

    // Check if we have anything to work on
    if (m_ready_task_indices.empty()) {
      // Keep track of idle cycles
      std::cout << "IDLE" << std::endl;
      m_idle_time += 1;
    } else {
      // Get the task to run
      Task &current_task = m_task_data[m_ready_task_indices.front()];
      if (current_task.time_on_cpu() == 0) {
        // Records the time of a task's first tick
        m_task_start_times.try_emplace(current_task.pid(), m_time_elapsed);
      }

      // Run one cycle of the task
      current_task.tick();
      // Hook for updating scheduler state if needed (round robin)
      on_task_tick(current_task);
    }
    m_time_elapsed += 1;
    return true;
  }

  void print_results() const {
    float num_tasks = static_cast<float>(m_task_data.size());
    if (num_tasks == 0) {
      std::cout << "No tasks to process." << std::endl;
      return;
    }

    // Calculate stats for this run
    float total_wait_time = 0.0;
    float total_turnaround_time = 0.0;
    float total_response_time = 0.0;
    for (auto const &[pid, last_cycle] : m_turnaround_times) {
      const Task *task_ptr = nullptr;
      for (const auto &t : m_task_data) {
        if (t.pid() == pid) {
          task_ptr = &t;
          break;
        }
      }
      if (!task_ptr)
        continue; // unreachable

      const Task &task = *task_ptr;
      size_t first_cycle = m_task_start_times.at(task.pid());

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

    std::cout << "\n--- Results ---\n"
              << "Avg Wait Time: " << avg_wait_time << std::endl
              << "Avg Response Time: " << avg_response_time << std::endl
              << "Avg Turnaround Time: " << avg_turnaround_time << std::endl
              << "CPU Utilization: " << cpu_utilization << std::endl
              << std::endl;
  }
};
// First Come First Serve
class FCFS : public Scheduler {
  // Add tasks to the ready list in the order of their arrival time
  void update_ready_list() override {
    handle_any_finished_task();

    auto tasks_to_add = get_newly_arrived_tasks();

    auto comp = [this](size_t a, size_t b) {
      auto &task_a = m_task_data[a];
      auto &task_b = m_task_data[b];
      if (task_a.arrival_time() != task_b.arrival_time()) {
        return task_a.arrival_time() < task_b.arrival_time();
      }
      return task_a.priority() < task_b.priority();
    };
    std::sort(tasks_to_add.begin(), tasks_to_add.end(), comp);

    for (size_t task_index : tasks_to_add) {
      m_ready_task_indices.push_back(task_index);
    }
  }
  // only make a scheduling decision when there is not a task running
  bool can_make_scheduling_decision() override {
    if (m_ready_task_indices.empty()) {
      return true;
    }
    return m_task_data[m_ready_task_indices.front()].time_left() == 0;
  }
};

// Shortest Job First (Non-Preemptive)
class SJF : public Scheduler {

  // Sort tasks in the ready list by their burst time
  void update_ready_list() override {
    handle_any_finished_task();

    auto tasks_to_add = get_newly_arrived_tasks();

    for (size_t task_index : tasks_to_add) {
      m_ready_task_indices.push_back(task_index);
    }

    auto comp = [this](size_t a, size_t b) {
      auto &task_a = m_task_data[a];
      auto &task_b = m_task_data[b];
      if (task_a.burst_time() != task_b.burst_time()) {
        return task_a.burst_time() < task_b.burst_time();
      }
      return task_a.priority() < task_b.priority();
    };

    std::sort(m_ready_task_indices.begin(), m_ready_task_indices.end(), comp);
  }
  // This algorithm is not preemptive, so we only make scheduling decisions when
  // there is no task running
  bool can_make_scheduling_decision() override {
    if (m_ready_task_indices.empty()) {
      return true;
    }
    return m_task_data[m_ready_task_indices.front()].time_left() == 0;
  }
};

// Preemptive Priority Scheduling
class PPS : public Scheduler {
  // Sort tasks in ready list by priority
  void update_ready_list() override {
    handle_any_finished_task();

    auto tasks_to_add = get_newly_arrived_tasks();

    for (size_t task_index : tasks_to_add) {
      m_ready_task_indices.push_back(task_index);
    }

    auto comp = [this](size_t a, size_t b) {
      auto &task_a = m_task_data[a];
      auto &task_b = m_task_data[b];
      if (task_a.priority() != task_b.priority()) {
        return task_a.priority() < task_b.priority();
      }
      return task_a.burst_time() < task_b.burst_time();
    };

    std::sort(m_ready_task_indices.begin(), m_ready_task_indices.end(), comp);
  }

  // This algorithm IS preemptive, so we also make scheduling decisions if a
  // task arrives with higher priority
  bool can_make_scheduling_decision() override {
    if (m_ready_task_indices.empty()) {
      return true;
    }

    Task &current_task = m_task_data[m_ready_task_indices.front()];
    if (current_task.time_left() == 0) {
      return true;
    }

    for (size_t task_index : m_not_arrived_task_indices) {
      Task &task = m_task_data[task_index];
      if (task.is_ready(m_time_elapsed) &&
          task.priority() < current_task.priority()) {
        return true;
      }
    }
    return false;
  }
};

// Round Robin
class RR : public Scheduler {
  size_t m_quantum;
  size_t m_current_slice;

  // Update the current slice time
  void on_task_tick(Task &_) override {
    // Suppress unused warning
    (void)_;
    m_current_slice++;
  }

  // Add new tasks to the ready list in FCFS order, but move the task at the
  // front of the list when its time slice ends
  void update_ready_list() override {
    bool task_finished = handle_any_finished_task();
    if (task_finished) {
      m_current_slice = 0;
    }

    auto tasks_to_add = get_newly_arrived_tasks();

    auto comp = [this](size_t a, size_t b) {
      return m_task_data[a].arrival_time() < m_task_data[b].arrival_time();
    };
    std::sort(tasks_to_add.begin(), tasks_to_add.end(), comp);

    for (size_t task_index : tasks_to_add) {
      m_ready_task_indices.push_back(task_index);
    }

    // Check if time slice is up
    bool slice_up = m_current_slice >= m_quantum;
    if (slice_up && !task_finished && !m_ready_task_indices.empty()) {
      m_ready_task_indices.push_back(m_ready_task_indices.front());
      m_ready_task_indices.erase(m_ready_task_indices.begin());
      m_current_slice = 0;
    }
  }
  // Make scheduling decisions when a time slice ends as well
  bool can_make_scheduling_decision() override {
    if (m_ready_task_indices.empty()) {
      return true;
    }
    Task &current_task = m_task_data[m_ready_task_indices.front()];
    if (current_task.time_left() == 0) {
      return true;
    }
    if (m_current_slice >= m_quantum) {
      return true;
    }
    for (size_t task_index : m_not_arrived_task_indices) {
      if (m_task_data[task_index].is_ready(m_time_elapsed)) {
        return true;
      }
    }
    return false;
  }

public:
  RR(size_t quantum) : m_quantum(quantum), m_current_slice(0) {
    if (m_quantum == 0)
      // Minimum quantum of 1
      m_quantum = 1;
  }
};

int main() {
  std::string input_data;
  while (1) {
    std::cout << "Enter the path for the input file: " << std::endl;
    std::string input_file_path;
    std::cin >> input_file_path;
    auto input_file = std::ifstream(input_file_path);
    if (!input_file.good()) {
      std::cout << "Error loading file" << std::endl;
    } else {
      input_data = std::string(std::istreambuf_iterator<char>(input_file),
                               std::istreambuf_iterator<char>());
      std::cout << "input_data: \n" << input_data << std::endl;
      break;
    }
  }

  Scheduler *scheduler;
  while (1) {
    std::cout << "Select which scheduler to use: (FCFS,SJF,PPS,RR)"
              << std::endl;

    std::string scheduler_selection;

    std::cin >> scheduler_selection;

    std::transform(scheduler_selection.begin(), scheduler_selection.end(),
                   scheduler_selection.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (scheduler_selection == "fcfs") {
      std::cout << "\nFCFS\n";
      scheduler = new FCFS();
      break;
    } else if (scheduler_selection == "sjf") {
      std::cout << "\nSJF\n";
      scheduler = new SJF();

      break;
    } else if (scheduler_selection == "pps") {

      std::cout << "\nPPS\n";
      scheduler = new PPS();

      break;
    } else if (scheduler_selection == "rr") {

      std::cout << "\nRR\n";
      float quantum;
      while (1) {
        std::cout << "Enter a quantum value: " << std::endl;
        std::cin >> quantum;
        if (quantum > 0.0) {
          break;
        } else {
          std::cout << "Quantum must be positive" << std::endl;
        }
      }
      size_t quantum_int = static_cast<size_t>(quantum);
      scheduler = new RR(quantum_int);
      break;
    } else {
      std::cout << "Invalid selection";
    }
  }

  scheduler->load_tasks(input_data);

  while (scheduler->tick()) {
  }
  scheduler->print_results();

  return 0;
}
