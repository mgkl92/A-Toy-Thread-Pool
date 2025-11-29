#pragma once

#include "thread_pool.h"

namespace mgkl {
ThreadPool::ThreadPool(uint nthreads) : m_running_{false} {
  assert(nthreads > 0 && "invalid parameter!");
  nthreads = std::min(nthreads, std::thread::hardware_concurrency());

  for (uint i = 0; i < nthreads; ++i) {
    m_threads_.emplace_back(std::bind(&ThreadPool::worker, this));
  }

  m_running_ = true;
}

void ThreadPool::worker() {
  while (true) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(m_mtx_);
      m_cond_.wait(lock, [this]() { return !m_tasks_.empty() || !m_running_; });

      if (!m_running_ && m_tasks_.empty()) {
        return;
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
    -> std::future<typename std::result_of<Func(Args...)>::type> {
  using return_type = typename std::result_of<Func(Args...)>::type;
  auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
      std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
  auto res = task_ptr->get_future();
  {
    std::unique_lock<std::mutex> lock(m_mtx_);
    if (!m_running_) {
      return {};
    } else {
      m_tasks_.push([task_ptr]() { (*task_ptr)(); });
    }
  }

  m_cond_.notify_one();

  return res;
}
}  // namespace mgkl
