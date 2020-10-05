#ifndef PROTOTYPE_ALPHA_TEXT_SPLITTER_H
#define PROTOTYPE_ALPHA_TEXT_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace alpha {
class TextSplitter : public SplitterBase {
 public:
  TextSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  void Split() override { std::cout << "TextSplitter::Split\n"; }
};
}  // namespace alpha

#endif