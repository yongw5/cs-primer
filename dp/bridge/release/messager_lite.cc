#include "messager_lite.h"
#include <iostream>
#include "platform.h"

namespace release {
void MessagerLite::Login(const std::string name, const std::string password) {
  platform_->Connect();
  std::cout << "MessagerLite::Login\n";
}
void MessagerLite::SendMessage(const std::string message) {
  platform_->WriteText();
  std::cout << "MessagerLite::SendMessage\n";
}
void MessagerLite::SendPicture(const std::string img_name) {
  platform_->DrawShape();
  std::cout << "MessagerLite::SendPicture\n";
}
}  // namespace release