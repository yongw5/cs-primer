#include "messager_perfect.h"

#include <iostream>

namespace beta {
void MessagerPerfect::Login(const std::string name,
                            const std::string password) {
  messager_->PlaySound();
  std::cout << "PCMessagerPerfect::Login\n";
  messager_->Connect();
}
void MessagerPerfect::SendMessage(const std::string message) {
  messager_->PlaySound();
  std::cout << "PCMessagerPerfect::SendMessage\n";
  messager_->WriteText();
}
void MessagerPerfect::SendPicture(const std::string img_name) {
  messager_->PlaySound();
  std::cout << "PCMessagerPerfect::SendPicture\n";
  messager_->DrawShape();
}
}  // namespace beta