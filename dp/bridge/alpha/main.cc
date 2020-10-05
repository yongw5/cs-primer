#include <iostream>
#include "messager.h"
#include "mobile_messager_lite.h"
#include "mobile_messager_perfect.h"
#include "pc_messager_lite.h"
#include "pc_messager_perfect.h"
using namespace std;
using namespace alpha;

#define Test(classtype)                       \
  do {                                        \
    cout << "Test " #classtype << endl;       \
    Messager* messager = new classtype();     \
    messager->Login("Messager", "password");  \
    messager->SendMessage("this is message"); \
    messager->SendPicture("image path.png");  \
    delete messager;                          \
    cout << '\n';                             \
  } while (0)

int main(int argc, char* argv[]) {
  Test(MobileMessagerLite);
  Test(MobileMessagerPerfect);
  Test(PCMessagerLite);
  Test(PCMessagerPerfect);
}
