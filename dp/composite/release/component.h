#ifndef COMPOSITE_RELEASE_COMPONENT_H
#define COMPOSITE_RELEASE_COMPONENT_H

namespace release {
class Component {
 public:
  virtual void Process() = 0;
  virtual ~Component() = default;
};
}  // namespace release

#endif