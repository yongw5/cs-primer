#ifndef FACTORY_METHOD_RELEASE_PICTURE_SPLITTER_FACTORY_H
#define FACTORY_METHOD_RELEASE_PICTURE_SPLITTER_FACTORY_H

#include "picture_splitter.h"
#include "splitter_factory.h"
namespace release {
class PictureSplitterFactory : public SplitterFactory {
 public:
  SplitterBase* CreateSplitter(const std::string file_path,
                               const int file_num) override {
    return new PictureSplitter(file_path, file_num);
  }
};
}  // namespace release

#endif