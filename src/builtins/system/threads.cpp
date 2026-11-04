#include "minishell/builtins/system/threads.h"

#include <dirent.h>

#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

std::mutex output_mutex;

bool is_number(const std::string &value) {
  return !value.empty() &&
         value.find_first_not_of("0123456789") == std::string::npos;
}

std::string read_thread_name(const std::string &path) {
  std::ifstream file(path + "/comm");
  std::string name;
  std::getline(file, name);
  return name;
}

bool list_threads(const std::string &pid) {
  std::string path = "/proc/" + pid + "/task";
  DIR *directory = opendir(path.c_str());
  if (directory == nullptr) {
    std::cerr << "threads: không tìm thấy tiến trình " << pid << '\n';
    return false;
  }

  std::cout << "TID        TÊN\n";
  while (dirent *entry = readdir(directory)) {
    std::string tid = entry->d_name;
    if (tid == "." || tid == "..")
      continue;
    std::cout << tid << "    " << read_thread_name(path + "/" + tid) << '\n';
  }

  closedir(directory);
  return true;
}

void worker(int id) {
  {
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << "luồng " << id << " bắt đầu\n";
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(200 * id));

  {
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << "luồng " << id << " kết thúc\n";
  }
}

void run_demo(int count) {
  std::vector<std::thread> workers;
  for (int id = 1; id <= count; ++id)
    workers.emplace_back(worker, id);
  for (std::thread &thread : workers)
    thread.join();
  std::cout << "demo xong\n";
}

int threads_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() == 3 && args[1] == "demo") {
    int count = 0;
    try {
      count = std::stoi(args[2]);
    } catch (const std::exception &) {
      count = 0;
    }

    if (count < 1 || count > 16) {
      std::cerr << "threads demo: N phải từ 1 đến 16\n";
      return 1;
    }
    run_demo(count);
    return 0;
  }

  if (args.size() == 2 && is_number(args[1])) {
    return list_threads(args[1]) ? 0 : 1;
  }

  if (args.size() == 2) {
    std::cerr << "threads: PID phải là số\n";
    return 1;
  }

  std::cerr << "cách dùng: threads PID | threads demo N\n";
  return 1;
}

} // namespace

void register_threads(Registry &registry) {
  registry.add("threads", threads_command,
               "threads PID | threads demo N  xem luồng / chạy demo");
}

} // namespace minishell::builtins
