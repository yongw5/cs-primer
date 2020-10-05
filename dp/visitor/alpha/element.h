#ifndef VISITOR_ALPHA_ELEMENT_H
#define VISITOR_ALPHA_ELEMENT_H

namespace alpha {
class Element {
 public:
  virtual void Accept() = 0;
  virtual ~Element() = default;
};
}  // namespace alpha

#endif