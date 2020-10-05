#include "console.h"
#include <cstdlib>
#include "file_splitter.h"
using std::string;

namespace beta {
void Console::ButtonClick() {
  string file_path = get_text("input file path to split: ");
  int file_num = atoi(get_text("input numbers of small files: ").c_str());
  FileSplitter fsplitter(file_path, file_num, bar_);
  fsplitter.Split();
}
std::string Console::get_text(const char *ptr) {
  if (ptr) {
    std::cout << ptr;
  }
  std::string tmp;
  std::cin >> tmp;
  return tmp;
}
}  // namespace beta
