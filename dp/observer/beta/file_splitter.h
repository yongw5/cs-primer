#ifndef OBSERVER_BETA_FILE_SPLITTER_H
#define OBSERVER_BETA_FILE_SPLITTER_H

#include <iostream>
#include <string>
namespace beta {
class ProgressBar;
class FileSplitter {
 public:
  FileSplitter(const std::string& file_path, int file_num, ProgressBar* bar);
  void Split();
  ~FileSplitter() {}

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
  ProgressBar* bar_;
};
}  // namespace beta
#endif
