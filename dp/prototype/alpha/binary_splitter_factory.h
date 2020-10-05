#ifndef PROTOTYPE_ALPHA_BINARY_SPLITTER_FACTORY_H
#define PROTOTYPE_ALPHA_BINARY_SPLITTER_FACTORY_H

#include "binary_splitter.h"
#include "splitter_factory.h"
namespace alpha {
class BinarySplitterFactory : public SplitterFactory {
 public:
  SplitterBase* CreateSplitter(const std::string file_path,
                               const int file_num) override {
    return new BinarySplitter(file_path, file_num);
  }
};
}  // namespace alpha

#endif