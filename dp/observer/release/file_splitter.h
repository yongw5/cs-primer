#ifndef OBSERVER_RELEASE_FILE_SPLITTER_H
#define OBSERVER_RELEASE_FILE_SPLITTER_H

#include <iostream>
#include <string>
#include <vector>

namespace release {
class IProgress;
class FileSplitter {
 public:
  FileSplitter(const std::string& file_path, int file_num);
  void Split();
  void AddProgress(IProgress* progress);
  void RemoveProgress(IProgress* progress);
  ~FileSplitter() {}

 protected:
  void OnProgress(float value);

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
  std::vector<IProgress*> progress_vec_;
};
}  // namespace release
#endif
