#ifndef BUILDER_RELEASE_HOUSE_BUILDER_H
#define BUILDER_RELEASE_HOUSE_BUILDER_H

namespace release {
class House;
class HouseBuilder {
 public:
  HouseBuilder(House* house) : house_(house) {}
  House* GetResult() { return house_; }
  virtual ~HouseBuilder() = default;

 protected:
  virtual bool BuildPart1() = 0;
  virtual bool BuildPart2() = 0;
  virtual bool BuildPart3() = 0;
  virtual bool BuildPart4() = 0;
  virtual bool BuildPart5() = 0;
  friend class HouseDirector;

  House* house_;
};
}  // namespace release

#endif