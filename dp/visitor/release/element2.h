#ifndef VISITOR_RELEASE_ELEMENT2_H
#define VISITOR_RELEASE_ELEMENT2_H

#include "element.h"
#include "visitor.h"
namespace release {
class Element2 : public Element {
 public:
  void Accept(Visitor* visitor) override { visitor->VisitElement2(this); }
};
}  // namespace release

#endif