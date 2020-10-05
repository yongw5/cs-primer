#ifndef PROTOTYPE_RELEASE_BINARY_SPLITTER_H
#define PROTOTYPE_RELEASE_BINARY_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace release {
class BinarySplitter : public SplitterBase {
 public:
  BinarySplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  SplitterBase* Clone() override { return new BinarySplitter(*this); }
  void Split() override { std::cout << "BinarySplitter::Split\n"; }
};
}  // namespace release

#endif