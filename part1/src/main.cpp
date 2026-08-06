#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <span>
#include <stdint.h>
#include <vector>

// Sorts sequentially at this threshold to limit the number of threads spawned
constexpr size_t SEQUENTIAL_THRESHOLD = 32;

// Merges two sorted vectors into a single sorted vector
std::vector<int> merge_vectors(const std::vector<int> &left,
                               const std::vector<int> &right) {
  std::vector<int> result;
  result.reserve(left.size() + right.size());
  std::merge(left.begin(), left.end(), right.begin(), right.end(),
             std::back_inserter(result));
  return result;
}

// Recursively sorts the input span, splitting into halves that are sorted in
// parallel via std::async
std::vector<int> parallel_merge_sort(std::span<const int> data) {
  if (data.size() <= SEQUENTIAL_THRESHOLD) {
    std::vector<int> output(data.begin(), data.end());
    std::sort(output.begin(), output.end());
    return output;
  }

  size_t split_len = data.size() / 2;
  std::span<const int> first_half = data.first(split_len);
  std::span<const int> second_half = data.subspan(split_len);

  auto left_future =
      std::async(std::launch::async, parallel_merge_sort, first_half);
  auto right_future =
      std::async(std::launch::async, parallel_merge_sort, second_half);

  return merge_vectors(left_future.get(), right_future.get());
}

int main() {
  const int N = 200;

  std::vector<int> data;
  data.reserve(N);
  for (int i = 0; i < N; i++) {
    data.push_back(i);
  }
  size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(data.begin(), data.end(), std::default_random_engine(seed));

  std::cout << "unsorted: ";
  for (int value : data) {
    std::cout << value << ' ';
  }

  std::cout << "\n\n";

  std::vector<int> result = parallel_merge_sort(data);

  std::cout << "sorted: ";
  for (int value : result) {
    std::cout << value << ' ';
  }

  return 0;
}
