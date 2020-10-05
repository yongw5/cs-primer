#ifndef OBSERVER_ALPHA_FILE_SPLITTER_H
#define OBSERVER_ALPHA_FILE_SPLITTER_H
#include <iostream>
#include <string>

namespace alpha {
class FileSplitter {
 public:
  FileSplitter(const std::string& file_path, int file_num);
  void Split();
  ~FileSplitter() {}

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
};
}

#endif
