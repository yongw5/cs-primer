#ifndef FACTORY_METHOD_ALPHA_VIDEO_SPLITTER_H
#define FACTORY_METHOD_ALPHA_VIDEO_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace alpha {
class VideoSplitter : public SplitterBase {
 public:
  VideoSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  void Split() override { std::cout << "VideoSplitter::Split\n"; }
};
}  // namespace alpha

#endif