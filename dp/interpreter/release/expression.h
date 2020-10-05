#ifndef INTERPRETER_RELEASE_EXPRESSION_H
#define INTERPRETER_RELEASE_EXPRESSION_H

#include <unordered_map>

namespace release {
class Expression {
 public:
  virtual int Interpret(std::unordered_map<char, int>& var) = 0;
  virtual ~Expression() = default;
};
}  // namespace release

#endif