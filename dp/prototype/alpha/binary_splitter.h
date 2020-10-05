#ifndef PROTOTYPE_ALPHA_BINARY_SPLITTER_H
#define PROTOTYPE_ALPHA_BINARY_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace alpha {
class BinarySplitter : public SplitterBase {
 public:
  BinarySplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  void Split() override { std::cout << "BinarySplitter::Split\n"; }
};
}  // namespace alpha

#endif