#ifndef COMMAND_RELEASE_CONCRETE_COMMAND1_H
#define COMMAND_RELEASE_CONCRETE_COMMAND1_H

#include <iostream>
#include <string>
#include "command.h"
#include "receiver.h"

namespace release {
class ConcreteCommand1 : public Command {
 public:
  ConcreteCommand1(Receiver* receiver, std::string& arg)
      : receiver_(receiver), arg_(arg) {}
  ConcreteCommand1(Receiver* receiver, const char* arg)
      : receiver_(receiver), arg_(arg) {}
  void Execute() override {
    std::cout << "ConcreteCommand1::Execute\n";
    receiver_->Action(arg_);
  }

 private:
  Receiver* receiver_;
  std::string arg_;
};
}  // namespace release

#endif