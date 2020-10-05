#ifndef PROTOTYPE_ALPHA_TEXT_SPLITTER_FACTORY_H
#define PROTOTYPE_ALPHA_TEXT_SPLITTER_FACTORY_H

#include "splitter_factory.h"
#include "text_splitter.h"
namespace alpha {
class TextSplitterFactory : public SplitterFactory {
 public:
  SplitterBase* CreateSplitter(const std::string file_path,
                               const int file_num) override {
    return new TextSplitter(file_path, file_num);
  }
};
}  // namespace alpha

#endif