#include "console.h"
#include <cstdlib>
#include "file_splitter.h"
#include "progress_bar.h"
using std::string;

namespace gamma {
void Console::ButtonClick() {
  string file_path = get_text("input file path to split: ");
  int file_num = atoi(get_text("input numbers of small files: ").c_str());
  FileSplitter fsplitter(file_path, file_num, this);
  fsplitter.Split();
}

void Console::DoProgress(float value) { bar_->SetValue(value); }

std::string Console::get_text(const char *ptr) {
  if (ptr) {
    std::cout << ptr;
  }
  std::string tmp;
  std::cin >> tmp;
  return tmp;
}
}  // namespace gamma
