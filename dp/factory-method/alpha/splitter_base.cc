#include "splitter_base.h"
#include <unistd.h>
#include <iostream>
using namespace std;

namespace alpha {
SplitterBase::SplitterBase(const string& file_path, int file_num)
    : file_path_(file_path), file_num_(file_num) {
  cout << "processing: " << file_path_ << endl;
  cout << "to split " << file_num_ << " small files" << endl;
}

void SplitterBase::ReadFile() {
  std::cout << "Reading file...";
  sleep(1);
}
}