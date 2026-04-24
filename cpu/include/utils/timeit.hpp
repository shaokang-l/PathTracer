#pragma once
#include <chrono>
#include <functional>
#include <iostream>

template <class Fn, class... Args> struct Timeit;

template <class Fn, class... Args> struct Timeit<Fn(Args...)> {
public:
  Timeit(std::function<Fn(Args...)> func) : f_(func){};
  std::function<Fn(Args...)> f_;

  void operator()(Args... args) {
    std::chrono::time_point<std::chrono::system_clock> start, end;
    std::chrono::duration<double> duration;

    start = std::chrono::system_clock::now();
    f_(args...);
    end = std::chrono::system_clock::now();
    duration = end - start;
    std::cout << duration.count() << " seconds" << std::endl;
  }
};

template <class Fn, class... Args>
Timeit<Fn(Args...)> make_decorator(Fn (*f)(Args...)) {
  return Timeit<Fn(Args...)>(std::function<Fn(Args...)>(f));
}
