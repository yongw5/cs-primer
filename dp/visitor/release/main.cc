#include "element.h"
#include "element1.h"
#include "element2.h"
#include "visitor1.h"
#include "visitor2.h"

using namespace release;

int main() {
  {
    Element* element = new Element2;
    Visitor* visitor = new Visitor2;
    element->Accept(visitor);
    delete element;
    delete visitor;
  }

  {
    Element* element = new Element1;
    Visitor* visitor = new Visitor2;
    element->Accept(visitor);
    delete element;
    delete visitor;
  }
}