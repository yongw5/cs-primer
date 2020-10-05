#ifndef PROTOTYPE_RELEASE_PICTURE_SPLITTER_H
#define PROTOTYPE_RELEASE_PICTURE_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace release {
class PictureSplitter : public SplitterBase {
 public:
  PictureSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  SplitterBase* Clone() override { return new PictureSplitter(*this); }
  void Split() override { std::cout << "PictureSplitter::Split\n"; }
};
}  // namespace release

#endif