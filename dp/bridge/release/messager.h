#ifndef BRIDGE_RELEASE_MESSAGER_H
#define BRIDGE_RELEASE_MESSAGER_H

#include <string>
namespace release {
class Platform;
class Messager {
 public:
  Messager(Platform* platform) : platform_(platform) {}
  virtual void Login(const std::string name, const std::string password) = 0;
  virtual void SendMessage(const std::string message) = 0;
  virtual void SendPicture(const std::string img_name) = 0;
  virtual ~Messager() = default;

 protected:
  Platform* platform_;
};
}  // namespace release
#endif