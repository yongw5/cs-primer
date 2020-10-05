#ifndef BRIDGE_RELEASE_MOBILE_MESSAGER_H
#define BRIDGE_RELEASE_MOBILE_MESSAGER_H

#include "platform.h"
namespace release {
class MobileMessager : public Platform {
 public:
  void PlaySound() override;
  void DrawShape() override;
  void WriteText() override;
  void Connect() override;
};
}  // namespace release
#endif