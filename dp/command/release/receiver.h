#ifndef COMMAND_RELEASE_RECEIVER_H
#define COMMAND_RELEASE_RECEIVER_H

#include <iostream>
#include <string>

namespace release {
class Receiver {
 public:
  void Action(std::string& arg) {
    std::cout << "Receiver::Action, " << arg << std::endl;
  }
};
}  // namespace release

#endif