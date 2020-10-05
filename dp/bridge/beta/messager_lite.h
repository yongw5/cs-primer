#ifndef BRIDGE_BETA_MESSAGER_LITE_H
#define BRIDGE_BETA_MESSAGER_LITE_H

#include "messager.h"
namespace beta {
class MessagerLite {
  void Login(const std::string name, const std::string password);
  void SendMessage(const std::string message);
  void SendPicture(const std::string img_name);

 private:
  Messager* messager_;
};
}  // namespace beta

#endif