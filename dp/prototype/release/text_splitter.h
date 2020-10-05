#ifndef PROTOTYPE_RELEASE_TEXT_SPLITTER_H
#define PROTOTYPE_RELEASE_TEXT_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace release {
class TextSplitter : public SplitterBase {
 public:
  TextSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  SplitterBase* Clone() override { return new TextSplitter(*this); }
  void Split() override { std::cout << "TextSplitter::Split\n"; }
};
}  // namespace release

#endif