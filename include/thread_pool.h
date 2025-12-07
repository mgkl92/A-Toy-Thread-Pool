#pragma once

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace mgkl {
#if __cplusplus >= 201703L ||                         \
    (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || \
    defined(__cpp_lib_invoke_result)
template <typename Func, typename... Args>
using result_of_t = std::invoke_result_t<Func, Args...>;
#else
template <typename Func, typename... Args>
using result_of_t = typename std::result_of<Func(Args...)>::type;
#endif

class ThreadPool {
 public:
  explicit ThreadPool(uint threads_cnt,
                      std::chrono::milliseconds = DEFAULT_IDLE_TIME);

  ~ThreadPool();

  ThreadPool(ThreadPool&) = delete;

  ThreadPool(ThreadPool&&) = delete;

  void resize(uint new_threads_cnt);

  uint get_thread_count() const;

  template <typename Func, typename... Args>
  auto submit(Func&& f, Args&&... args)
      -> std::future<result_of_t<Func, Args...>>;

  static constexpr std::chrono::milliseconds DEFAULT_IDLE_TIME{30 * 1000};

 private:
  void worker();
  std::queue<std::function<void()>> m_tasks_;
  std::vector<std::thread> m_threads_;
  std::mutex m_mtx_;
  std::condition_variable m_cond_;
  bool m_running_;
  std::atomic<uint> m_ready_cnt_;
  uint m_current_threads_cnt_;
  uint m_min_threads_cnt_;
  std::chrono::milliseconds m_idle_timeout;
  uint m_threads_cnt_to_remove_;
};
}  // namespace mgkl