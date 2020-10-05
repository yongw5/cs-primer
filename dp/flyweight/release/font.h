#ifndef FLYWEIGHT_RELEASE_FONT_H
#define FLYWEIGHT_RELEASE_FONT_H

#include <string>
namespace release {
class Font {
 public:
  Font(const std::string& key) : key_(key) {}

 private:
  std::string key_;
};
}  // namespace release

#endif