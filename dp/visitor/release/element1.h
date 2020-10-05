#ifndef VISITOR_RELEASE_ELEMENT1_H
#define VISITOR_RELEASE_ELEMENT1_H

#include "element.h"
#include "visitor.h"

namespace release {
class Element1 : public Element {
 public:
  void Accept(Visitor* visitor) override { visitor->VisitElement1(this); }
};
}  // namespace release

#endif