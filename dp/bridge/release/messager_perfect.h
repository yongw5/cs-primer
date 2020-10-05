#ifndef BRIDGE_RELEASE_MESSAGER_PERFECT_H
#define BRIDGE_RELEASE_MESSAGER_PERFECT_H

#include "messager.h"
namespace release {
class Platform;
class MessagerPerfect : public Messager {
 public:
  MessagerPerfect(Platform* platform) : Messager(platform) {}
  void Login(const std::string name, const std::string password) override;
  void SendMessage(const std::string message) override;
  void SendPicture(const std::string img_name) override;
};
}  // namespace release
#endif