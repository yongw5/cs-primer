#ifndef VISITOR_RELEASE_ELEMENT_H
#define VISITOR_RELEASE_ELEMENT_H

namespace release {
class Visitor;
class Element {
 public:
  virtual void Accept(Visitor* visitor) = 0;
  virtual ~Element() = default;
};
}  // namespace release

#endif