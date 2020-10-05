#ifndef TEMPLATE_PATTERN_RELEASE_APPLICATION_H
#define TEMPLATE_PATTERN_RELEASE_APPLICATION_H
#include "library.h"

namespace release {
class Application : public Library {
 public:
  Application();
  bool Step2();
  bool Step4();
  virtual ~Application();
};
}  // namespace release
#endif
