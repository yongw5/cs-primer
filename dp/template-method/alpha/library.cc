#include "library.h"
#include <iostream>

namespace alpha {
Library::Library() { std::cout << "Library Construction" << std::endl; }
bool Library::Step1() {
  std::cout << "Library Step1" << std::endl;
  return true;
}
bool Library::Step3() {
  std::cout << "Library Step3" << std::endl;
  return true;
}
bool Library::Step5() {
  std::cout << "Library Step5" << std::endl;
  return true;
}
Library::~Library() { std::cout << "Library Deconstruction" << std::endl; }
}  // namespace alpha
