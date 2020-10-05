#ifndef VISITOR_RELEASE_VISITOR1_H
#define VISITOR_RELEASE_VISITOR1_H

#include <iostream>
#include "visitor.h"
namespace release {
class Visitor1 : public Visitor {
 public:
  void VisitElement1(Element* element) override {
    std::cout << "Visitor1::VisitElement1\n";
  }
  void VisitElement2(Element* element) override {
    std::cout << "Visitor1::VisitElement2\n";
  }
};
}  // namespace release

#endif