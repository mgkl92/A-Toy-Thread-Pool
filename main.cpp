#include <iostream>

#include "thread_pool.hpp"

int main(int argc, char const* argv[]) {
  auto f = [](int i) {
    std::cout << "Id is " << i << std::endl;
    return 0;
  };
  mgkl::ThreadPool pool(2);

  for (size_t i = 0; i < 10; ++i) {
    pool.submit(f, i);
  }

  return 0;
}
