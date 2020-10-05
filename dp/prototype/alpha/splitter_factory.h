#ifndef PROTOTYPE_ALPHA_SPLITTER_FACTORY_H
#define PROTOTYPE_ALPHA_SPLITTER_FACTORY_H
#include "splitter_base.h"
namespace alpha {
class SplitterFactory {
 public:
  virtual SplitterBase* CreateSplitter(const std::string file_path,
                                       const int file_num) = 0;
  virtual ~SplitterFactory() = default;
};
}  // namespace alpha
#endif