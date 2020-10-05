#ifndef BUILDER_RELEASE_STONE_HOUSE_BUILDER_H
#define BUILDER_RELEASE_STONE_HOUSE_BUILDER_H

#include <iostream>
#include "house_builder.h"

namespace release {
class House;
class StoneHouseBuilder : public HouseBuilder {
 public:
  StoneHouseBuilder(House* house) : HouseBuilder(house) {}

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
}  // namespace release

#endif