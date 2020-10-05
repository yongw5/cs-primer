#include "console.h"
#include <cstdlib>
using std::string;

namespace alpha {
void Console::ButtonClick() {
  string file_path = get_text("input file path to split: ");
  int file_num = atoi(get_text("input numbers of small files: ").c_str());
  SplitterBase* splitter = nullptr
      /* new BinarySplitter(file_path, file_num) 编译依赖 */;
  splitter->Split();
}

std::string Console::get_text(const char* ptr) {
  if (ptr) std::cout << ptr;
  std::string tmp;
  std::cin >> tmp;
  return tmp;
}
}  // namespace alpha