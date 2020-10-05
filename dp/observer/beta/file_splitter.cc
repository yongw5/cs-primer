#include "file_splitter.h"
#include <unistd.h>
#include <iostream>
#include <string>
#include "progress_bar.h"
using std::cout;
using std::endl;
using std::string;

namespace beta {
FileSplitter::FileSplitter(const string& file_path, int file_num,
                           ProgressBar* bar)
    : file_path_(file_path), file_num_(file_num), bar_(bar) {
  cout << "processing: " << file_path_ << endl;
  cout << "to split " << file_num_ << " small files" << endl;
}
void FileSplitter::Split() {
  ReadFile();
  cout << "processing...";
  for (int i = 0; i < file_num_; ++i) {
    if (bar_ != nullptr) {
      bar_->SetValue(float(i + 1) / file_num_);
      usleep(3000);
    }
  }
  cout << endl;
}
void FileSplitter::ReadFile() {
  std::cout << "Reading file..." << std::endl;
  sleep(1);
}
}  // namespace beta
