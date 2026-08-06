# CPU Scheduling & Parallel Sorting

An operating systems class project containing two parts: a parallel merge sort built on
std::async, and a CPU scheduler simulation implementing four classic scheduling policies.

## Part 1: Parallel Merge Sort

Part 1 is a recursive merge sort that splits the input in half and sorts each half
concurrently with std::async before merging the two sorted halves back together.

- Uses std::span for non-owning views into the input and move semantics for
  transferring results between tasks.
- Demoed on a shuffled 0..199 array.

## Part 2: CPU Scheduler Simulation

CPU scheduling simulation that reads a workload of tasks and drives a
scheduler through a tick() method, CPU usage and scheduling
metrics on completion. 

Implemented policies:
First-Come First-Served (FCFS): tasks run in arrival order, non-preemptive. 
Shortest Job First (SJF): runs the task with the smallest burst time, non-preemptive. 
Preemptive Priority Scheduling (PPS): highest-priority task always runs, preempts when a higher-priority task arrives. 
Round Robin (RR)): each task runs for a fixed quantum before yielding, time-sliced. 

Metrics: average wait time, average response time, average turnaround
time, and CPU utilization.

### Input format

A file with one task per line:

pid, arrival_time, burst_time, priority

Example from part2/input.txt:

```
1 13 20 18
2 25 35 2
3 15 10 6
```

## Build

Requires CMake >= 4.1 and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build
```

Binaries are created in bin/.

## Usage

```sh
# Part 1
./bin/part1

# Part 2 (scheduler selection: FCFS, SJF, PPS, RR)
./bin/part2
```

Part 2 prompts for the input file path, then the scheduler to use. Round
Robin additionally asks for a quantum value.

## Concepts

- Concurrency and task parallelism (std::async, futures)
- Real-time scheduling concepts (preemption, time slicing, priority queues)
- Object-oriented design via polymorphism and virtual dispatch
- Performance metric analysis (turnaround, response, wait, utilization)
