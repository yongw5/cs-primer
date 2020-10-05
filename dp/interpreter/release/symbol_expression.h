#ifndef INTERPRETER_RELEASE_SYMBOL_EXPRESSION_H
#define INTERPRETER_RELEASE_SYMBOL_EXPRESSION_H

#include "expression.h"

namespace release {
class SymbolExpression : public Expression {
 public:
  SymbolExpression(Expression* left, Expression* right)
      : left_(left), right_(right) {}
  Expression* right() const { return right_; }
  Expression* left() const { return left_; }

 protected:
  Expression *left_, *right_;
};
}  // namespace release

#endif