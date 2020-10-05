#include <iostream>
#include <string>
#include <unordered_map>
#include "expression.h"
#include "utils.h"

using namespace std;
using namespace release;

int main() {
  string exp_str = "a+b-c+d";
  unordered_map<char, int> var;
  var.insert(make_pair('a', 5));
  var.insert(make_pair('b', 2));
  var.insert(make_pair('c', 1));
  var.insert(make_pair('d', 6));
  Expression* expression = Analyse(exp_str);
  int result = expression->Interpret(var);
  Release(expression);
  cout << result << endl;
  return 0;
}