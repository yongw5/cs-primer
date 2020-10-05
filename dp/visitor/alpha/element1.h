#ifndef VISITOR_ALPHA_ELEMENT1_H
#define VISITOR_ALPHA_ELEMENT1_H

#include <iostream>
#include "element.h"
namespace alpha {
class Element1 : public Element {
 public:
  void Accept() override { std::cout << "Element1::Accept\n"; }
};
}  // namespace alpha

#endif