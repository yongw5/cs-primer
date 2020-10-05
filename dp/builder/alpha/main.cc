#include "house.h"
#include "stone_house.h"

int main() {
  alpha::House* house = new alpha::StoneHouse();
  house->Init();
  delete house;
}