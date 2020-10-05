#ifndef PROTOTYPE_ALPHA_PICTURE_SPLITTER_H
#define PROTOTYPE_ALPHA_PICTURE_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace alpha {
class PictureSplitter : public SplitterBase {
 public:
  PictureSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  void Split() override { std::cout << "PictureSplitter::Split\n"; }
};
}  // namespace alpha

#endif