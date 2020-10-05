#ifndef VISITOR_RELEASE_VISITOR_H
#define VISITOR_RELEASE_VISITOR_H

namespace release {
class Element;
class Visitor {
 public:
  virtual void VisitElement1(Element* element) = 0;
  virtual void VisitElement2(Element* element) = 0;
  virtual ~Visitor() = default;
};
}  // namespace release

#endif