#ifndef INTERPRETER_RELEASE_UTILS_H
#define INTERPRETER_RELEASE_UTILS_H

#include <string>
#include "expression.h"
namespace release {
Expression* Analyse(const std::string& exp_str);
void Release(Expression* Expression);
}

#endif