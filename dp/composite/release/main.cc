#include "component.h"
#include "composite.h"
#include "leaf.h"
using namespace release;

#define DEF_COMPOSITE(valariable) Composite valariable(#valariable)
#define DEF_LEAF(valariable) Leaf valariable(#valariable)

int main() {
  DEF_COMPOSITE(root);
  DEF_COMPOSITE(treenode1);
  DEF_COMPOSITE(treenode2);
  DEF_COMPOSITE(treenode3);
  DEF_COMPOSITE(treenode4);

  DEF_LEAF(leaf1);
  DEF_LEAF(leaf2);

  root.Add(&treenode1);
  treenode1.Add(&treenode2);
  treenode2.Add(&leaf1);

  root.Add(&treenode3);
  treenode3.Add(&treenode4);
  treenode4.Add(&leaf2);

  root.Process();
}