#ifndef INTERPRETER_RELEASE_VAR_EXPRESSION_H
#define INTERPRETER_RELEASE_VAR_EXPRESSION_H

#include "expression.h"

namespace release {
class VarExpression : public Expression {
 public:
  VarExpression(const char key) : key_(key) {}
  int Interpret(std::unordered_map<char, int>& var) override {
    return var[key_];
  }

 private:
  char key_;
};
}  // namespace release

#endif