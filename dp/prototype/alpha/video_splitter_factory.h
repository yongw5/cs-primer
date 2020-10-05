#ifndef PROTOTYPE_ALPHA_VIDEO_SPLITTER_FACTORY_H
#define PROTOTYPE_ALPHA_VIDEO_SPLITTER_FACTORY_H

#include "splitter_factory.h"
#include "video_splitter.h"
namespace alpha {
class VideoSplitterFactory : public SplitterFactory {
 public:
  SplitterBase* CreateSplitter(const std::string file_path,
                               const int file_num) override {
    return new VideoSplitter(file_path, file_num);
  }
};
}  // namespace alpha

#endif