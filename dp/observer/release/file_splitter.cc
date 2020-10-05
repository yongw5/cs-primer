#include "file_splitter.h"
#include <unistd.h>
#include <iostream>
#include <string>
#include "iprogress.h"
using std::cout;
using std::endl;
using std::string;

namespace release {
FileSplitter::FileSplitter(const string& file_path, int file_num)
    : file_path_(file_path), file_num_(file_num) {
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

void FileSplitter::AddProgress(IProgress* progress) {
  progress_vec_.push_back(progress);
}
void FileSplitter::RemoveProgress(IProgress* progress) {
  std::vector<IProgress*>::iterator to_remove = progress_vec_.end();
  for (auto it = progress_vec_.begin(); it != progress_vec_.end(); ++it) {
    if (*it == progress) {
      to_remove = it;
      break;
    }
  }
  progress_vec_.erase(to_remove);
}

void FileSplitter::OnProgress(float value) {
  for (auto p : progress_vec_) {
    if (p != nullptr) {
      p->DoProgress(value);
      usleep(3000);
    }
  }
}

void FileSplitter::ReadFile() {
  std::cout << "Reading file..." << std::endl;
  sleep(1);
}
}  // namespace release
