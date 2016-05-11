#include <iostream>
#include "threadpool.h"

int fibo(int a) {
  if (a < 2) {
    return 1;
  }
  return fibo(a - 1) + fibo(a - 2);
}

void write() { std::cout << fibo(24) << std::endl; }

void writenr(int a) {
  std::cout << a << " ... " << "curthread:" << std::this_thread::get_id() << std::endl;
}

int main(int argc, const char *argv[]) {
  ThreadPool pool;

  for (int i = 0; i < 10; ++i) {
    pool(write);
    pool(std::bind(writenr, i));
    pool([]() { std::cout << "hej" << std::endl; });
  }
}
