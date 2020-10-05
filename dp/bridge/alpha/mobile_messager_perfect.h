#ifndef BRIDGE_ALPHA_MOBILE_MESSAGER_PERFECT_H
#define BRIDGE_ALPHA_MOBILE_MESSAGER_PERFECT_H

#include "mobile_messager_base.h"
namespace alpha {
class MobileMessagerPerfect : public MobileMessagerBase {
  void Login(const std::string name, const std::string password) override;
  void SendMessage(const std::string message) override;
  void SendPicture(const std::string img_name) override;
};
}  // namespace alpha

#endif