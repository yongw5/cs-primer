#ifndef BRIDGE_ALPHA_MESSAGER_H
#define BRIDGE_ALPHA_MESSAGER_H

#include <string>
namespace alpha {
class Messager {
 public:
  virtual void Login(const std::string name, const std::string password) = 0;
  virtual void SendMessage(const std::string message) = 0;
  virtual void SendPicture(const std::string img_name) = 0;
  virtual void PlaySound() = 0;
  virtual void DrawShape() = 0;
  virtual void WriteText() = 0;
  virtual void Connect() = 0;
  virtual ~Messager() = default;
};
}  // namespace alpha
#endif