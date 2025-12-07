#pragma once

#include "thread_pool.h"

namespace mgkl {
constexpr std::chrono::milliseconds ThreadPool::DEFAULT_IDLE_TIME;

ThreadPool::ThreadPool(uint nthreads, std::chrono::milliseconds idle_timeout)
    : m_running_{true},
      m_ready_cnt_{0},
      m_idle_timeout{idle_timeout},
      m_threads_cnt_to_remove_{0} {
  assert(nthreads > 0 && "invalid parameter!");
  nthreads = std::min(nthreads, std::thread::hardware_concurrency());

  for (uint i = 0; i < nthreads; ++i) {
    m_threads_.emplace_back(std::bind(&ThreadPool::worker, this));
  }
  m_current_threads_cnt_ = static_cast<uint>(m_threads_.size());
  m_min_threads_cnt_ = m_current_threads_cnt_;

  // waiting for all worker threads to be ready
  while (m_ready_cnt_.load() < nthreads) {
    std::this_thread::yield();
  }
}

inline void mgkl::ThreadPool::resize(uint new_threads_cnt) {
  std::lock_guard<std::mutex> lock(m_mtx_);

  if (!m_running_ || (new_threads_cnt == m_current_threads_cnt_)) {
    return;
  }

  assert(new_threads_cnt > 0 && "Invalid new thread count");
  new_threads_cnt =
      std::min(new_threads_cnt, std::thread::hardware_concurrency());
  if (new_threads_cnt > m_current_threads_cnt_) {
    uint threads_cnt_to_add = new_threads_cnt - m_current_threads_cnt_;
    for (uint i = 0; i < threads_cnt_to_add; ++i) {
      m_threads_.emplace_back(std::bind(&ThreadPool::worker, this));
    }

    m_current_threads_cnt_ = m_threads_.size();
    m_min_threads_cnt_ = m_current_threads_cnt_;
  } else {
    m_min_threads_cnt_ = new_threads_cnt;
    m_cond_.notify_all();
  }

  m_current_threads_cnt_ = new_threads_cnt;
}

inline uint mgkl::ThreadPool::get_thread_count() const {
  return m_current_threads_cnt_;
}

void ThreadPool::worker() {
  m_ready_cnt_++;

  while (true) {
    std::function<void()> task;
    bool is_timeout = false;
    {
      std::unique_lock<std::mutex> lock(m_mtx_);
      m_cond_.wait_for(lock, m_idle_timeout,
                       [this]() { return !m_tasks_.empty() || !m_running_; });

      if (!m_running_ && m_tasks_.empty()) {
        return;
      }

      if (is_timeout && m_current_threads_cnt_ > m_min_threads_cnt_) {
        --m_current_threads_cnt_;
        return;
      }

      if (m_tasks_.empty()) {
        continue;
      }

      task = std::move(m_tasks_.front());
      m_tasks_.pop();
    }

    task();
  }
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(m_mtx_);
    m_running_ = false;
  }

  m_cond_.notify_all();
  for (auto& t : m_threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
}

template <typename Func, typename... Args>
auto ThreadPool::submit(Func&& f, Args&&... args)
    -> std::future<result_of_t<Func, Args...>> {
  using return_type = result_of_t<Func, Args...>;
  auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
  auto res = task_ptr->get_future();
  {
    std::unique_lock<std::mutex> lock(m_mtx_);
    if (!m_running_) {
      return res;
    } else {
      m_tasks_.push([task_ptr]() { (*task_ptr)(); });
    }
  }

  m_cond_.notify_one();

  return res;
}
}  // namespace mgkl
