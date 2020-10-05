#ifndef FACTORY_METHOD_RELEASE_PICTURE_SPLITTER_H
#define FACTORY_METHOD_RELEASE_PICTURE_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace release {
class PictureSplitter : public SplitterBase {
 public:
  PictureSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  void Split() override { std::cout << "PictureSplitter::Split\n"; }
};
}  // namespace release

#endif