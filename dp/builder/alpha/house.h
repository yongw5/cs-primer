#ifndef BUILDER_ALPHA_HOUSE_H
#define BUILDER_ALPHA_HOUSE_H

namespace alpha {
class House {
 public:
  void Init() {
    this->BuildPart1();
    for (int i = 0; i < 4; ++i) {
      this->BuildPart2();
    }
    bool flag = this->BuildPart3();
    if (flag) {
      this->BuildPart4();
    }
    this->BuildPart5();
  }
  virtual ~House() = default;

 protected:
  virtual bool BuildPart1() = 0;
  virtual bool BuildPart2() = 0;
  virtual bool BuildPart3() = 0;
  virtual bool BuildPart4() = 0;
  virtual bool BuildPart5() = 0;
};
}  // namespace alpha

#endif