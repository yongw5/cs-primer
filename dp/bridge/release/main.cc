#include <iostream>
#include "messager.h"
#include "messager_lite.h"
#include "messager_perfect.h"
#include "mobile_messager.h"
#include "pc_messager.h"
#include "platform.h"

using namespace std;
using namespace release;

#define Test(flatform, version)                      \
  do {                                               \
    cout << "Test " #flatform #version << endl;      \
    Platform* imp = new flatform();                  \
    Messager* messager = new Messager##version(imp); \
    messager->Login("Messager", "password");         \
    messager->SendMessage("this is message");        \
    messager->SendPicture("image path.png");         \
    delete messager;                                 \
    delete imp;                                      \
    cout << '\n';                                    \
  } while (0)

int main(int argc, char* argv[]) {
  Test(MobileMessager, Lite);
  Test(MobileMessager, Perfect);
  Test(PCMessager, Lite);
  Test(PCMessager, Perfect);
}
