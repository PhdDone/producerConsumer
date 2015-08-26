#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include <sstream>
#include <mutex>
#include <condition_variable>

std::mutex mut;
std::mutex omut;
std::queue<int> data_queue;
std::condition_variable data_cond;
thread_local unsigned int ps = 0;
thread_local std::stringstream pd;

void data_preparation_thread() {
  int i = 0;
  while (i < 100) {
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(i);
    data_cond.notify_one();
    i++;
  }
}

void data_process_thread(int id, int num_of_item) {
  for (int i = 0; i < num_of_item; ++i) {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, []{return !data_queue.empty();});
    int data = data_queue.front();
    data_queue.pop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lk.unlock();
    //processData(data);
    pd << id << " processes data" << data << std::endl;
    //    std::cout << id << " processes data: " << data << std::endl;
    ++ps;
    if (ps > 9) {
      std::unique_lock<std::mutex> olk(omut);
      std::cout << pd.str();
      olk.unlock();
      pd.str("");
      ps = 0;
      std::cout << "@@@@@@@@@@@@@@@@@@@@" << std::endl;
    }
  }
}

int main() {
  std::thread prod1(data_preparation_thread);

  // consumer threads
  std::thread consumer1(data_process_thread, 1, 50);
  std::thread consumer2(data_process_thread, 2, 50);
  //std::thread consumer3(data_process_thread, 3);
  prod1.join();
  consumer1.join();
  consumer2.join();
  //consumer3.join();
  return 0;
}
