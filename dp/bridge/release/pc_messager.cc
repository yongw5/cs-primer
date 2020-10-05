#include "pc_messager.h"
#include <iostream>
namespace release {
void PCMessager::PlaySound() { std::cout << "PCMessager::PlaySound\n"; }
void PCMessager::DrawShape() { std::cout << "PCMessager::DrawShape\n"; }
void PCMessager::WriteText() { std::cout << "PCMessager::WriteText\n"; }
void PCMessager::Connect() { std::cout << "PCMessager::Connect\n"; }
}  // namespace release