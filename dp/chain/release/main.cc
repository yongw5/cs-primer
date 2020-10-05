#include "handler1.h"
#include "handler2.h"
#include "handler3.h"
#include "request.h"

using namespace release;
using namespace std;

int main() {
  Handler1 h1;
  Handler2 h2;
  Handler3 h3;
  h1.set_next(&h2);
  h2.set_next(&h3);

  Request req("process task...", RequestType::REQ_HANDLER3);
  h1.Handle(req);
  return 0;
}