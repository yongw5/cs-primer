#ifndef PROTOTYPE_RELEASE_SPLITTER_BASE_H
#define PROTOTYPE_RELEASE_SPLITTER_BASE_H

#include <string>
namespace release {
class SplitterBase {
 public:
  SplitterBase(const std::string file_path, int file_num);
  virtual SplitterBase* Clone() = 0;
  virtual void Split() = 0;
  virtual ~SplitterBase() = default;

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
};
}  // namespace release

#endif