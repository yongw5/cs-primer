#ifndef FACTORY_METHOD_ALPHA_SPLITTER_BASE_H
#define FACTORY_METHOD_ALPHA_SPLITTER_BASE_H

namespace alpha {
class SplitterBase {
 public:
  SplitterBase(const std::string& file_path, int file_num);
  virtual void Split() = 0;
  virtual ~SplitterBase() = default;

 private:
  void ReadFile();
  std::string file_path_;
  int file_num_;
};
}  // namespace alpha

#endif