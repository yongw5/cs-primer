#include "utils.h"
#include <stack>
#include "add_expression.h"
#include "sub_expression.h"
#include "var_expression.h"

namespace release {
Expression* Analyse(const std::string& exp_str) {
  std::stack<Expression*> exp_stack;
  Expression *left = nullptr, *right = nullptr;
  for (int i = 0; i < exp_str.size(); ++i) {
    switch (exp_str[i]) {
      case '+':
        left = exp_stack.top();
        right = new VarExpression(exp_str[++i]);
        exp_stack.push(new AddExpression(left, right));
        break;
      case '-':
        left = exp_stack.top();
        right = new VarExpression(exp_str[++i]);
        exp_stack.push(new SubExpression(left, right));
        break;
      default:
        exp_stack.push(new VarExpression(exp_str[i]));
    }
  }
  return exp_stack.top();
}

void Release(Expression* exp) {
  if (exp) {
    if (VarExpression* p = dynamic_cast<VarExpression*>(exp)) {
      delete exp;
    } else if (SymbolExpression* p = dynamic_cast<SymbolExpression*>(exp)) {
      Release(p->left());
      Release(p->right());
      delete p;
    }
  }
}

}  // namespace release