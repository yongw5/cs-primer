#include "house.h"
#include "house_director.h"
#include "stone_house.h"
#include "stone_house_builder.h"

int main() {
  release::House* house = new release::StoneHouse();
  release::StoneHouseBuilder builder(house);
  release::HouseDirector director(&builder);
  director.Construct();
  delete house;
}