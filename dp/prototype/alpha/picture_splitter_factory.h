#ifndef PROTOTYPE_ALPHA_PICTURE_SPLITTER_FACTORY_H
#define PROTOTYPE_ALPHA_PICTURE_SPLITTER_FACTORY_H

#include "picture_splitter.h"
#include "splitter_factory.h"
namespace alpha {
class PictureSplitterFactory : public SplitterFactory {
 public:
  SplitterBase* CreateSplitter(const std::string file_path,
                               const int file_num) override {
    return new PictureSplitter(file_path, file_num);
  }
};
}  // namespace alpha

#endif