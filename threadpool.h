#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

class ThreadPool {
private:
  std::mutex lock;
  std::condition_variable longwait;
  unsigned active_threads;
  bool shutdown;
  std::queue<std::function<void()>> tasks;
  std::vector<std::thread> threads;

  /**
   * For each thread; fetches and runs a task, or waits.
   */
  void thread_entry();

  ThreadPool(ThreadPool const &rhs) = delete;
  ThreadPool(ThreadPool &&rhs) = delete;
  ThreadPool &operator=(ThreadPool const &rhs) = delete;
  ThreadPool &operator=(ThreadPool &&rhs) = delete;

public:
  /**
   * CTOR creates threads, defaults to 0 -> detection of cpus.
   */
  ThreadPool(unsigned = 0);

  /**
   * DTOR waits for completion of all tasks.
   */
  ~ThreadPool();

  /**
   * Enqueues a task.
   */
  void operator()(std::function<void()>);

  /**
   * Continuously sleeps until done() is true.
   */
  void wait_until_done();

  /**
   * Returns whether all tasks are finished.
   */
  bool done();
};

void ThreadPool::thread_entry() {
  std::function<void()> job;
  while (true) {
    {
      std::unique_lock<std::mutex> l(lock);

      while (tasks.empty()) {
        if (shutdown) {
          return;
        } else {
          --active_threads;
          longwait.wait(l);
          ++active_threads;
        }
      }

      job = std::move(tasks.front());
      tasks.pop();
    }

    job();
  }
}

ThreadPool::ThreadPool(unsigned number_of_threads) : shutdown(false) {
  if (number_of_threads == 0) {
    number_of_threads = std::thread::hardware_concurrency();

    // Failure of hardware_concurrency()
    if (number_of_threads == 0) {
      number_of_threads = 4;
    }
  }
  active_threads = number_of_threads;

  for (unsigned i = 0; i < number_of_threads; ++i) {
    threads.emplace_back(std::bind(&ThreadPool::thread_entry, this));
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> l(lock);
    shutdown = true;
  }
  longwait.notify_all();

  // threads will not join while !tasks.empty().
  for (auto &thread : threads) {
    thread.join();
  }
}

void ThreadPool::operator()(std::function<void()> func) {
  {
    std::unique_lock<std::mutex> l(lock);
    tasks.push(std::move(func));
  }
  longwait.notify_one();
}

void ThreadPool::wait_until_done() {
  while (!done()) {
    std::this_thread::yield();
  }
}

bool ThreadPool::done() {
  std::unique_lock<std::mutex> l(lock);
  return (tasks.empty() && !active_threads);
}

#endif /* end of include guard: THREADPOOL_H */
