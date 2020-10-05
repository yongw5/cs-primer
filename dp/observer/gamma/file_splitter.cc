#include "file_splitter.h"
#include <unistd.h>
#include <iostream>
#include <string>
#include "iprogress.h"
using std::cout;
using std::endl;
using std::string;

namespace gamma {
FileSplitter::FileSplitter(const string& file_path, int file_num,
                           IProgress* iprogress)
    : file_path_(file_path), file_num_(file_num), iprogress_(iprogress_) {
  cout << "processing: " << file_path_ << endl;
  cout << "to split " << file_num_ << " small files" << endl;
}
void FileSplitter::Split() {
  ReadFile();
  cout << "processing..." << std::endl;
  for (int i = 0; i < file_num_; ++i) {
    OnProgress(float(i + 1) / file_num_);
  }
  cout << endl;
}

void FileSplitter::OnProgress(float value) {
  if (iprogress_ != nullptr) {
    iprogress_->DoProgress(value);
    usleep(3000);
  }
}

void FileSplitter::ReadFile() {
  std::cout << "Reading file..." << std::endl;
  sleep(1);
}
}  // namespace gamma
