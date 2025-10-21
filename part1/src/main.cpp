#include <algorithm>
#include <chrono>
#include <cstdio>
#include <future>
#include <random>
#include <span>
#include <stdint.h>
#include <vector>

// Takes a promise object 'promise' and a std::span 'input_span', places a
// vector into 'promise' containing the sorted contents of 'input span'
void sort_function(std::promise<std::vector<int>> &&promise,
                   std::span<int> &&input_span) {
  auto output_vector = std::vector(input_span.begin(), input_span.end());
  std::sort(output_vector.begin(), output_vector.end());
  promise.set_value(std::move(output_vector));
  return;
}
// Takes a promise object 'promise' and two ownership of two input vectors
// 'first_half' and 'second_half' places a vector containing the sorted contents
// of both input vectors into 'promise'
void merge_function(std::promise<std::vector<int>> &&promise,
                    std::vector<int> &&first_half,
                    std::vector<int> &&second_half) {

  std::vector<int> result;
  result.reserve(first_half.size() + second_half.size());

  std::merge(first_half.begin(), first_half.end(), second_half.begin(),
             second_half.end(), std::back_inserter(result));

  promise.set_value(std::move(result));

  return;
}

// Takes an input span of ints 'data' , returns a vector containing the sorted
// contents of data
std::vector<int> multithreaded_sort(std::span<int> data) {

  if (data.empty()) {
    return std::vector<int>();
  }

  size_t split_len = data.size() / 2;

  auto begin_split_1 = data.begin();
  auto end_split_1 = data.begin() + split_len;

  auto begin_split_2 = end_split_1;
  auto end_split_2 = data.end();
  std::span<int> first_half = std::span(begin_split_1, end_split_1);
  std::span<int> second_half = std::span(begin_split_2, end_split_2);

  std::promise<std::vector<int>> sort_thread_1_promise, sort_thread_2_promise;

  auto thread_1_future = sort_thread_1_promise.get_future();

  auto thread_2_future = sort_thread_2_promise.get_future();

  std::thread sort_thread_1 = std::thread(
      &sort_function, std::move(sort_thread_1_promise), std::move(first_half));

  std::thread sort_thread_2 = std::thread(
      &sort_function, std::move(sort_thread_2_promise), std::move(second_half));

  sort_thread_1.join();
  sort_thread_2.join();

  auto sorted_first_half = thread_1_future.get();

  auto sorted_second_half = thread_2_future.get();

  std::promise<std::vector<int>> merge_thread_promise;

  auto merge_thread_future = merge_thread_promise.get_future();

  std::thread merge_thread =
      std::thread(&merge_function, std::move(merge_thread_promise),
                  std::move(sorted_first_half), std::move(sorted_second_half));

  merge_thread.join();

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
  std::printf("unsorted:");
  for (int value : data) {
    std::printf("%d ", value);
  }

  std::printf("\n\n");

  std::vector<int> result = multithreaded_sort(data);

  // print our sorted result
  printf("sorted: ");
  for (int value : result) {
    std::printf("%d ", value);
  }

  std::printf("\n");

  return 0;
}
