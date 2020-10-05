#include "application.h"
#include "library.h"

int main() {
  release::Library *lib = new release::Application();  //多态
  lib->Run();
  delete lib;
  return 0;
}
