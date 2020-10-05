#ifndef TEMPLATE_PATTERN_ALPHA_LIBRARY_H
#define TEMPLATE_PATTERN_ALPHA_LIBRARY_H

namespace alpha {
class Library {
 public:
  Library();
  bool Step1();
  bool Step3();
  bool Step5();
  virtual ~Library();
};
}  // namespace alpha
#endif