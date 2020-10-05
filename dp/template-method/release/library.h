#ifndef TEMPLATE_PATTERN_RELEASE_LIBRARY_H
#define TEMPLATE_PATTERN_RELEASE_LIBRARY_H

namespace release {
class Library {
 public:
  Library();
  // 稳定部分 Run, Step1, Step3, Step5
  void Run();

 protected:
  bool Step1();
  bool Step3();
  bool Step5();
  // 变换部分 Step2 Step4
  virtual bool Step2() = 0;
  virtual bool Step4() = 0;

 public:
  virtual ~Library();
};
}  // namespace release
#endif
