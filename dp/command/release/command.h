#ifndef COMMAND_RELEASE_COMMAND_H
#define COMMAND_RELEASE_COMMAND_H

namespace release {
class Command {
 public:
  virtual void Execute() = 0;
};
}  // namespace release

#endif