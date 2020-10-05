#include "messager_lite.h"
#include <iostream>

namespace beta {
void MessagerLite::Login(const std::string name, const std::string password) {
  messager_->Connect();
  std::cout << "PCMessagerBase::Login\n";
}
void MessagerLite::SendMessage(const std::string message) {
  messager_->WriteText();
  std::cout << "PCMessagerBase::SendMessage\n";
}
void MessagerLite::SendPicture(const std::string img_name) {
  messager_->DrawShape();
  std::cout << "PCMessagerBase::SendPicture\n";
}
}  // namespace beta