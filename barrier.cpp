#include <chrono>
#include <condition_variable>
#include <mutex>

class Barrier {
  const int threshold;
  int waiters;
  unsigned generation;

  std::mutex mutex;
  std::condition_variable new_generation;

public:
  explicit Barrier(int threshold);

  void wait();

  enum WaitResult { RELEASE, TIMEOUT };
  WaitResult wait(std::chrono::steady_clock::time_point deadline);
};

Barrier::Barrier(int threshold)
: threshold(threshold)
, waiters(0)
, generation(0) {}

void Barrier::wait() {
  std::unique_lock<std::mutex> lock{mutex};
  const unsigned my_generation = generation;
  ++waiters;
  if (waiters == threshold) {
    ++generation;
    waiters = 0;
    lock.unlock();
    new_generation.notify_all();
    return;
  }
  new_generation.wait(lock, [&]() { return generation != my_generation; });
}

Barrier::WaitResult Barrier::wait(std::chrono::steady_clock::time_point deadline) {
  std::unique_lock<std::mutex> lock{mutex};
  const unsigned my_generation = generation;
  ++waiters;
  if (waiters == threshold) {
    ++generation;
    waiters = 0;
    lock.unlock();
    new_generation.notify_all();
    return RELEASE;
  }
  const bool new_gen = new_generation.wait_until(lock, deadline,
   [&]() { return generation != my_generation; });
  if (new_gen) {
    return RELEASE;
  }
  --waiters;
  return TIMEOUT;
}

#include <iostream>
#include <thread>

int main() {
  const auto begin = std::chrono::steady_clock::now();
  const int iterations = 3;
  Barrier barrier{3};

  for (int i = 0; i < iterations; ++i) {
    std::thread t1{[&]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      barrier.wait();
    }};

    std::thread t2{[&]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      Barrier::WaitResult result;
      do {
        result = barrier.wait(std::chrono::steady_clock::now() + std::chrono::milliseconds(100));
      } while (result == Barrier::TIMEOUT);
    }};

    std::thread t3{[&]() {
      std::this_thread::sleep_for(std::chrono::seconds(4));
      barrier.wait();
    }};

    t1.join();
    t2.join();
    t3.join();
  }

  std::cout << "did " << iterations << " rounds in "
    << std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - begin).count()
    << " seconds\n";
}
