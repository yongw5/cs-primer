#ifndef FACTORY_METHOD_RELEASE_BINARY_SPLITTER_H
#define FACTORY_METHOD_RELEASE_BINARY_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace release {
class BinarySplitter : public SplitterBase {
 public:
  BinarySplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  void Split() override { std::cout << "BinarySplitter::Split\n"; }
};
}  // namespace release

#endif