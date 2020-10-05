#include "application.h"
#include <iostream>

namespace release {
Application::Application() {
  std::cout << "Application Construction" << std::endl;
}
bool Application::Step2() {
  std::cout << "Application Step2" << std::endl;
  return true;
}
bool Application::Step4() {
  std::cout << "Application Step4" << std::endl;
  return true;
}
Application::~Application() {
  std::cout << "Application Deconstruction" << std::endl;
}
}