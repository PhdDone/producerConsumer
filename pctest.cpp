#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>

//#include <boost/thread/shared_mutex.hpp>
//#include <boost/unordered_map.hpp>

std::mutex mut;
std::mutex omut;
std::mutex entry_mutex;

std::queue<std::string> data_queue;
bool finished= false;
std::condition_variable data_cond;
thread_local unsigned int ps = 0;
thread_local std::stringstream pd;

typedef std::unordered_map<std::string, int> FeatureMap;
FeatureMap fmap;

std::ofstream finalOutput; //update std::cout to this later

//std::ostream& finalOutput = std::cout;

void data_preparation_thread(const char* filename) {
  int i = 0;
  std::string line;
  std::ifstream iFile(filename);
  while (std::getline(iFile, line)) {
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(line);
    data_cond.notify_one();
  }
  //TODO: try less time
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  std::lock_guard<std::mutex> lk(mut);
  finished = true;
  data_cond.notify_all();
}

void buildFeature(std::vector<std::string> const& tokens) {
  std::vector<std::pair<int, int> > out_bin;
  //std::lock_guard<std::mutex> lk(fmut);
  //std::lock_guard<std::mutex> lk(entry_mutex);
  std::unique_lock<std::mutex> lk(entry_mutex);
  for (std::size_t i = 4; i < tokens.size(); i += 2) {
    //boost::shared_lock<boost::shared_mutex> slk(entry_mutex);
    FeatureMap::const_iterator mIter = fmap.find(tokens[i]);
    if (mIter != fmap.end()) {
      out_bin.push_back(std::make_pair(mIter->second, i));
    }
    else {
      int index = fmap.size() + 1;
      fmap[tokens[i]] = index;
      out_bin.push_back(std::make_pair(index, i));
    }
  }
  lk.unlock();
  sort(out_bin.begin(),
       out_bin.end(),
       [](const std::pair<int, int>& a, const std::pair<int, int>& b){ return a.first < b.first; });

  for(std::size_t i = 0; i < out_bin.size(); ++i)
    pd << " " << out_bin[i].first << ":" << tokens[out_bin[i].second + 1];
}

void parseLine(std::string const& line) {
  std::string output;
  std::vector<std::string> tokens;
  std::stringstream ss(line);
  std::string item;
  char del = ' ';
  while (std::getline(ss, item, del)) {
    tokens.push_back(item);
  }
  pd << tokens[2] << " qid:" << tokens[0];
  buildFeature(tokens);
  pd << std::endl;
}

void processData(const std::string& data, int cache_size) {
  parseLine(data);
  //pd << " processes data: " << data << std::endl;
  //    std::cout << id << " processes data: " << data << std::endl;
  ++ps;
  if (ps >= cache_size) {
    std::unique_lock<std::mutex> olk(omut);
    finalOutput << pd.str();
    olk.unlock();
    pd.str("");
    ps = 0;
  }
}

void data_process_thread(int id, int cache_size) {
  while (true) {
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk, []{ return !data_queue.empty() || finished;});
    if (finished) break;
    std::string data = data_queue.front();
    data_queue.pop();
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    lk.unlock();
    processData(data, cache_size);
  }
  //dump cache
  std::lock_guard<std::mutex> olk(omut);
  finalOutput << pd.str();
  //olk.unlock();
}

void printFeature(const char* fileName) {
  std::vector<std::string> fnames(fmap.size());
  for (FeatureMap::const_iterator it = fmap.begin(); it != fmap.end(); ++it) {
    fnames[it->second - 1] = it->first;
  }
  std::ofstream oFile(fileName);
  for (std::size_t i=0; i < fnames.size(); ++i) {
    oFile << fnames[i] << std::endl;
  }
  oFile.close();
}

int main(int argc, char** argv) {
  finalOutput.open(argv[2], std::ofstream::out);
  std::thread prod1(data_preparation_thread, argv[1]);

  // consumer threads
  //TODO: try larger cache and more thread
  std::thread consumer1(data_process_thread, 1, 10);
  std::thread consumer2(data_process_thread, 2, 10);
  std::thread consumer3(data_process_thread, 3, 10);
  std::thread consumer4(data_process_thread, 4, 10);

  prod1.join();
  consumer1.join();
  consumer2.join();
  consumer3.join();
  consumer4.join();
  std::cout << "main thread process: " << data_queue.size() << std::endl;
  while (!data_queue.empty()) {
    std::string data = data_queue.front();
    data_queue.pop();
    processData(data, 0);
  }

  printFeature(argv[3]);
  //TODO: dump featurename
  return 0;
}
