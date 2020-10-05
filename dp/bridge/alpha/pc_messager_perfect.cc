#include "pc_messager_perfect.h"

#include <iostream>

namespace alpha {
void PCMessagerPerfect::Login(const std::string name,
                              const std::string password) {
  PCMessagerBase::PlaySound();
  std::cout << "PCMessagerPerfect::Login\n";
  PCMessagerBase::Connect();
}
void PCMessagerPerfect::SendMessage(const std::string message) {
  PCMessagerBase::PlaySound();
  std::cout << "PCMessagerPerfect::SendMessage\n";
  PCMessagerBase::WriteText();
}
void PCMessagerPerfect::SendPicture(const std::string img_name) {
  PCMessagerBase::PlaySound();
  std::cout << "PCMessagerPerfect::SendPicture\n";
  PCMessagerBase::DrawShape();
}
}  // namespace alpha