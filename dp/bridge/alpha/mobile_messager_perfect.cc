#include "mobile_messager_perfect.h"

#include <iostream>

namespace alpha {
void MobileMessagerPerfect::Login(const std::string name,
                                  const std::string password) {
  MobileMessagerBase::PlaySound();
  std::cout << "MobileMessagerPerfect::Login\n";
  MobileMessagerBase::Connect();
}
void MobileMessagerPerfect::SendMessage(const std::string message) {
  MobileMessagerBase::PlaySound();
  std::cout << "MobileMessagerPerfect::SendMessage\n";
  MobileMessagerBase::WriteText();
}
void MobileMessagerPerfect::SendPicture(const std::string img_name) {
  MobileMessagerBase::PlaySound();
  std::cout << "MobileMessagerPerfect::SendPicture\n";
  MobileMessagerBase::DrawShape();
}
}  // namespace alpha