#include "messager_perfect.h"
#include <iostream>
#include "platform.h"

namespace release {
void MessagerPerfect::Login(const std::string name,
                            const std::string password) {
  platform_->PlaySound();
  std::cout << "MessagerPerfect::Login\n";
  platform_->Connect();
}
void MessagerPerfect::SendMessage(const std::string message) {
  platform_->PlaySound();
  std::cout << "MessagerPerfect::SendMessage\n";
  platform_->WriteText();
}
void MessagerPerfect::SendPicture(const std::string img_name) {
  platform_->PlaySound();
  std::cout << "MessagerPerfect::SendPicture\n";
  platform_->DrawShape();
}
}  // namespace release