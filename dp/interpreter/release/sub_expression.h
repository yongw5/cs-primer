#ifndef INTERPRETER_RELEASE_SUB_EXPRESSION_H
#define INTERPRETER_RELEASE_SUB_EXPRESSION_H

#include "symbol_expression.h"

namespace release {
class SubExpression : public SymbolExpression {
 public:
  SubExpression(Expression* left, Expression* right)
      : SymbolExpression(left, right) {}
  int Interpret(std::unordered_map<char, int>& var) override {
    return left_->Interpret(var) - right_->Interpret(var);
  }
};
}  // namespace release

#endif