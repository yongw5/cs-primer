#ifndef OBSERVER_GAMMA_FILE_SPLITTER_H
#define OBSERVER_GAMMA_FILE_SPLITTER_H

#include <iostream>
#include <string>
namespace gamma {
class IProgress;
class FileSplitter {
 public:
  FileSplitter(const std::string& file_path, int file_num, IProgress* iprogress_);
  void Split();
  ~FileSplitter() {}

  protected:
  void OnProgress(float value);

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
  IProgress* iprogress_;
};
}  // namespace gamma
#endif
