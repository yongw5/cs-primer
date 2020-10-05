#ifndef VISITOR_ALPHA_ELEMENT2_H
#define VISITOR_ALPHA_ELEMENT2_H

#include <iostream>
#include "element.h"
namespace alpha {
class Element2 : public Element {
 public:
  void Accept() override { std::cout << "Element2::Accept\n"; }
};
}  // namespace alpha

#endif