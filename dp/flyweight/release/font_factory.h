#ifndef FLYWEIGHT_RELEASE_FONT_FACTORY_H
#define FLYWEIGHT_RELEASE_FONT_FACTORY_H

#include <unordered_map>
#include "font.h"
namespace release {
class FontFactory {
 public:
  Font* GetFont(const std::string& key) {
    auto item = font_pool_.find(key);
    if (item != font_pool_.end()) {
      return font_pool_[key];
    } else {
      Font* font = new Font(key);
      font_pool_[key] = font;
      return font;
    }
  }
  void clear() {
    ///...
  }

 private:
  std::unordered_map<std::string, Font*> font_pool_;
};
}  // namespace release
#endif