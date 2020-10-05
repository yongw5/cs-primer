#include "console.h"
#include <cstdlib>
#include <iostream>
#include "splitter_factory.h"
using std::string;

namespace release {
void Console::ButtonClick() {
  string file_path = get_text("input file path to split: ");
  int file_num = atoi(get_text("input numbers of small files: ").c_str());
  SplitterBase* splitter = factory_->CreateSplitter(file_path, file_num);
  splitter->Split();
}

std::string Console::get_text(const char* ptr) {
  if (ptr) std::cout << ptr;
  std::string tmp;
  std::cin >> tmp;
  return tmp;
}
}  // namespace release