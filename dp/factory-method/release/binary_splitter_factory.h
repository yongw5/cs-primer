#ifndef FACTORY_METHOD_RELEASE_BINARY_SPLITTER_FACTORY_H
#define FACTORY_METHOD_RELEASE_BINARY_SPLITTER_FACTORY_H

#include "binary_splitter.h"
#include "splitter_factory.h"
namespace release {
class BinarySplitterFactory : public SplitterFactory {
 public:
  SplitterBase* CreateSplitter(const std::string file_path,
                               const int file_num) override {
    return new BinarySplitter(file_path, file_num);
  }
};
}  // namespace release

#endif