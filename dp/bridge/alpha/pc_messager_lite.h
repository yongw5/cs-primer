#ifndef BRIDGE_ALPHA_PC_MESSAGER_LITE_H
#define BRIDGE_ALPHA_PC_MESSAGER_LITE_H

#include "pc_messager_base.h"
namespace alpha {
class PCMessagerLite : public PCMessagerBase {
  void Login(const std::string name, const std::string password) override;
  void SendMessage(const std::string message) override;
  void SendPicture(const std::string img_name) override;
};
}  // namespace alpha

#endif