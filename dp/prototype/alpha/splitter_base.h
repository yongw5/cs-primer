#ifndef PROTOTYPE_ALPHA_SPLITTER_BASE_H
#define PROTOTYPE_ALPHA_SPLITTER_BASE_H

#include <string>
namespace alpha {
class SplitterBase {
 public:
  SplitterBase(const std::string file_path, int file_num);
  virtual void Split() = 0;
  virtual ~SplitterBase() = default;

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
};
}  // namespace alpha

#endif