#include "file_splitter.h"
#include <unistd.h>
#include <iostream>
#include <string>
using std::cout;
using std::endl;
using std::string;

namespace alpha {
FileSplitter::FileSplitter(const string& file_path, int file_num)
    : file_path_(file_path), file_num_(file_num) {
  cout << "processing: " << file_path_ << endl;
  cout << "to split " << file_num_ << " small files" << endl;
}
void FileSplitter::Split() {
  ReadFile();
  cout << "processing...";
  for (int i = 0; i < file_num_; ++i) {
    // do split;
    usleep(3000);
  }
  cout << endl;
}

void FileSplitter::ReadFile() {
  std::cout << "Reading file...";
  sleep(1);
}
}