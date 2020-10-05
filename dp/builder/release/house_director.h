#ifndef BUILDER_RELEASE_HOUSE_DIRECTOR_H
#define BUILDER_RELEASE_HOUSE_DIRECTOR_H

#include "house.h"
#include "house_builder.h"
namespace release {
class HouseDirector {
 public:
  HouseDirector(HouseBuilder* builder) : builder_(builder) {}
  House* Construct() {
    builder_->BuildPart1();
    for (int i = 0; i < 4; ++i) {
      builder_->BuildPart2();
    }
    bool flag = builder_->BuildPart3();
    if (flag) {
      builder_->BuildPart4();
    }
    builder_->BuildPart5();
    return builder_->GetResult();
  }

 private:
  HouseBuilder* builder_;
};
}  // namespace release

#endif