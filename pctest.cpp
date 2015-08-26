#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <string>

std::mutex mut;
std::mutex omut;
std::queue<std::string> data_queue;
std::condition_variable data_cond;
thread_local unsigned int ps = 0;
thread_local std::stringstream pd;

void data_preparation_thread(const char* filename) {
  int i = 0;
  std::string line;
  std::ifstream iFile(filename);
  while (std::getline(iFile, line)) {
    std::cout <<"PUSH: " << line << std::endl;
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(line);
    data_cond.notify_one();
  }
}

void processData(const std::string& data, int cache_size) {
  pd << " processes data: " << data << std::endl;
  //    std::cout << id << " processes data: " << data << std::endl;
  ++ps;
  if (ps > cache_size) {
    std::unique_lock<std::mutex> olk(omut);
    std::cout << pd.str();
    olk.unlock();
    pd.str("");
    ps = 0;
    std::cout << "@@@@@@@@@@@@@@@@@@@@" << std::endl;
  }
}

void data_process_thread(int id, int num_of_item, int cache_size) {
  for (int i = 0; i < num_of_item; ++i) {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, []{return !data_queue.empty();});
    std::string data = data_queue.front();
    data_queue.pop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lk.unlock();
    processData(data, cache_size);
  }
  //dump cache
  std::unique_lock<std::mutex> olk(omut);
  std::cout << pd.str();
  olk.unlock();
}

int main(int argc, char** argv) {
  std::thread prod1(data_preparation_thread, argv[1]);

  // consumer threads
  std::thread consumer1(data_process_thread, 1, 4, 2);
  std::thread consumer2(data_process_thread, 2, 4, 2);
  //std::thread consumer3(data_process_thread, 3);
  prod1.join();
  consumer1.join();
  consumer2.join();
  std::cout << data_queue.size() << std::endl;
  while (!data_queue.empty()) {
    std::string data = data_queue.front();
    data_queue.pop();
    std::cout << "main thread: " << " processes data: " << data << std::endl;
  }
  //consumer3.join();
  return 0;
}
