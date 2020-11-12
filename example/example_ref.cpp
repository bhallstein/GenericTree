#define _GTR_ENABLE_SERIALIZATION

#include "Diatom-Storage.h"

Diatom _toDiatom(int x) {
  Diatom d = (double)x;
  return d;
}
int _fromDiatom(Diatom d) {
  return d.number_value();
}

void _print(int d) {
  printf("%d", d);
}

#include "GenericTree_Referential.h"

int main() {
  GenericTree_Referential<int> tree;

  int i0 = tree.addNode(0);
  int i1 = tree.addNode(1, i0);
  int i2 = tree.addNode(2, i0);
  int i3 = tree.addNode(3, i0);
  int i4 = tree.addNode(4, i2);
  int i5 = tree.addNode(5, i2);
  tree.print();

  Diatom d = tree.toDiatom();

  GenericTree_Referential<int> tree2;
  tree2.fromDiatom(d);
  tree2.print();

  tree2.walk([](int x, int i, int indent) {
    printf("%d ", x);
  });
  printf("\n");

  return 0;
}

