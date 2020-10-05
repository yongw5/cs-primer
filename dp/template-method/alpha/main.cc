#include "library.h"
#include "application.h"

int main() {
  alpha::Library lib;
  alpha::Application app;
  lib.Step1();
  if(app.Step2()) {
    lib.Step3();
  }
  for(int i =0; i < 4; ++i)
    app.Step4();
  lib.Step5();
  return 0;
}
