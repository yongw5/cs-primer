#ifndef BUILDER_ALPHA_STONE_HOUSE_H
#define BUILDER_ALPHA_STONE_HOUSE_H

#include <iostream>
#include "house.h"

namespace alpha {
class StoneHouse : public House {
 protected:
  bool BuildPart1() override {
    std::cout << "StoneHouse::BuildPart1\n";
    return true;
  }
  bool BuildPart2() override {
    std::cout << "StoneHouse::BuildPart2\n";
    return true;
  }
  bool BuildPart3() override {
    std::cout << "StoneHouse::BuildPart3\n";
    return true;
  }
  bool BuildPart4() override {
    std::cout << "StoneHouse::BuildPart4\n";
    return true;
  }
  bool BuildPart5() override {
    std::cout << "StoneHouse::BuildPart5\n";
    return true;
  }
};
}  // namespace alpha
#endif