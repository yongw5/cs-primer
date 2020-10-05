#ifndef COMMAND_RELEASE_MACRO_COMMAND_H
#define COMMAND_RELEASE_MACRO_COMMAND_H

#include <iostream>
#include <string>
#include <vector>
#include "command.h"

namespace release {
class MacorCommand : public Command {
 public:
  void AddCommand(Command* command) { commands_.push_back(command); }
  void Execute() override {
    for (auto& c : commands_) {
      c->Execute();
    }
  }

 private:
  std::vector<Command*> commands_;
};
}  // namespace release

#endif