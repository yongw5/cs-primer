#ifndef FACTORY_METHOD_RELEASE_SPLITTER_FACTORY_H
#define FACTORY_METHOD_RELEASE_SPLITTER_FACTORY_H
#include "splitter_base.h"
namespace release {
class SplitterFactory {
 public:
  virtual SplitterBase* CreateSplitter(const std::string file_path,
                                       const int file_num) = 0;
  virtual ~SplitterFactory() = default;
};
}  // namespace release
#endif