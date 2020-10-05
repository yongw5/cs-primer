#include "concrete_command1.h"
#include "concrete_command2.h"
#include "macro_command.h"
#include "receiver.h"

using namespace release;
int main() {
  Receiver receiver;
  ConcreteCommand1 command1(&receiver, "Arg ###");
  ConcreteCommand2 command2(&receiver, "Arg $$$");

  MacorCommand macro;
  macro.AddCommand(&command1);
  macro.AddCommand(&command2);
  macro.Execute();
}