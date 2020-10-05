#ifndef VISITOR_RELEASE_VISITOR2_H
#define VISITOR_RELEASE_VISITOR2_H

#include <iostream>
#include "visitor.h"
namespace release {
class Visitor2 : public Visitor {
 public:
  void VisitElement1(Element* element) override {
    std::cout << "Visitor2::VisitElement1\n";
  }
  void VisitElement2(Element* element) override {
    std::cout << "Visitor2::VisitElement2\n";
  }
};
}  // namespace release

#endif