#ifndef PROTOTYPE_RELEASE_VIDEO_SPLITTER_H
#define PROTOTYPE_RELEASE_VIDEO_SPLITTER_H

#include <iostream>
#include "splitter_base.h"

namespace release {
class VideoSplitter : public SplitterBase {
 public:
  VideoSplitter(const std::string file_path, const int file_num)
      : SplitterBase(file_path, file_num) {}
  SplitterBase* Clone() override { return new VideoSplitter(*this); }
  void Split() override { std::cout << "VideoSplitter::Split\n"; }
};
}  // namespace release

#endif