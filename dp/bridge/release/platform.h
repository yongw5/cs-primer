#ifndef BRIDGE_RELEASE_PLATFORM_H
#define BRIDGE_RELEASE_PLATFORM_H

namespace release {
class Platform {
 public:
  virtual void PlaySound() = 0;
  virtual void DrawShape() = 0;
  virtual void WriteText() = 0;
  virtual void Connect() = 0;
  virtual ~Platform() = default;
};
}  // namespace release
#endif