#ifndef BRIDGE_RELEASE_PC_MESSAGER_H
#define BRIDGE_RELEASE_PC_MESSAGER_H

#include "platform.h"
namespace release {
class PCMessager : public Platform {
 public:
  void PlaySound() override;
  void DrawShape() override;
  void WriteText() override;
  void Connect() override;
};
}  // namespace release

#endif