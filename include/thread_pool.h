#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace mgkl {
class ThreadPool {
 public:
  explicit ThreadPool(uint nthreads);

  ~ThreadPool();

  ThreadPool(ThreadPool&) = delete;

  ThreadPool(ThreadPool&&) = delete;

  template <typename Func, typename... Args>
  auto submit(Func&& f, Args&&... args)
      -> std::future<typename std::result_of<Func(Args...)>::type>;

 private:
  void worker();
  std::queue<std::function<void()>> m_tasks_;
  std::vector<std::thread> m_threads_;
  std::mutex m_mtx_;
  std::condition_variable m_cond_;
  bool m_running_;
};
}  // namespace mgkl