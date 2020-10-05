#ifndef INTERPRETER_RELEASE_ADD_EXPRESSION_H
#define INTERPRETER_RELEASE_ADD_EXPRESSION_H

#include "symbol_expression.h"

namespace release {
class AddExpression : public SymbolExpression {
 public:
  AddExpression(Expression* left, Expression* right)
      : SymbolExpression(left, right) {}
  int Interpret(std::unordered_map<char, int>& var) override {
    return left_->Interpret(var) + right_->Interpret(var);
  }
};
}  // namespace release

#endif