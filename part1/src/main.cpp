#include <algorithm>
#include <chrono>
#include <cstdio>
#include <future>
#include <random>
#include <span>
#include <stdint.h>
#include <vector>

// Creates a copy of the input data, sorts it, and moves the results into a
// promise
void sort_function(std::promise<std::vector<int>> &&promise,
                   std::span<int> &&input_span) {
  // Create a copy of our input data
  auto output_vector = std::vector(input_span.begin(), input_span.end());
  // Sort the data
  std::sort(output_vector.begin(), output_vector.end());
  // Move the result into our promise
  promise.set_value(std::move(output_vector));
  return;
}
// Awaits the results of our two sorting threads and then merges and moves
// their results into a promise
void merge_function(std::promise<std::vector<int>> &&promise,
                    std::future<std::vector<int>> &&thread_1_future,
                    std::future<std::vector<int>> &&thread_2_future) {

  std::vector<int> result;

  // Wait for our sorting threads to finish and get the results
  thread_1_future.wait();
  thread_2_future.wait();
  std::vector<int> thread_1_data = thread_1_future.get();
  std::vector<int> thread_2_data = thread_2_future.get();

  // Merge and sort the two sorted arrays
  std::merge(thread_1_data.begin(), thread_1_data.end(), thread_2_data.begin(),
             thread_2_data.end(), std::back_inserter(result));

  // Move our final array into the promise
  promise.set_value(std::move(result));

  return;
}

// Takes an input span of ints 'data' , returns a vector containing the sorted
// contents of data
std::vector<int> multithreaded_sort(std::span<int> data) {

  // No data to sort
  if (data.empty()) {
    return std::vector<int>();
  }

  // Split the input span in two
  size_t split_len = data.size() / 2;
  auto begin_split_1 = data.begin();
  auto end_split_1 = data.begin() + split_len;
  auto begin_split_2 = end_split_1;
  auto end_split_2 = data.end();
  std::span<int> first_half = std::span(begin_split_1, end_split_1);
  std::span<int> second_half = std::span(begin_split_2, end_split_2);

  // Create promise objects for thread synchronization
  std::promise<std::vector<int>> sort_thread_1_promise, sort_thread_2_promise;

  // Get the future objects for getting the data back out of the thread
  auto thread_1_future = sort_thread_1_promise.get_future();
  auto thread_2_future = sort_thread_2_promise.get_future();

  // Spawn our sorting threads, detaching them so that we don't need to .join()
  std::thread(&sort_function, std::move(sort_thread_1_promise),
              std::move(first_half))
      .detach();
  std::thread(&sort_function, std::move(sort_thread_2_promise),
              std::move(second_half))
      .detach();

  // Synchronization objects for our merging thread
  std::promise<std::vector<int>> merge_thread_promise;
  auto merge_thread_future = merge_thread_promise.get_future();

  // Spawn the merging thread
  std::thread(&merge_function, std::move(merge_thread_promise),
              std::move(thread_1_future), std::move(thread_2_future))
      .detach();

  // Await and return the results
  merge_thread_future.wait();
  return merge_thread_future.get();
}

int main() {
  std::vector<int> data = std::vector<int>();

  const int N = 200;

  // fill our input array with the values 0..N-1 and then shuffle it
  for (int i = 0; i < N; i++) {
    data.push_back(i);
  }
  size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(data.begin(), data.end(), std::default_random_engine(seed));

  // print our unsorted vector
  std::printf("unsorted: ");
  for (int value : data) {
    std::printf("%d ", value);
  }

  std::printf("\n\n");

  // Call our sorting function
  std::vector<int> result = multithreaded_sort(data);

  // print our sorted result
  printf("sorted: ");
  for (int value : result) {
    std::printf("%d ", value);
  }

  return 0;
}
