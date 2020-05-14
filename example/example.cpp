#define _GT_ENABLE_SERIALIZATION

#include "GenericTree.h"
#include "Diatom-Storage.h"
#include <cassert>

int main() {

  int x0 = 0;
  int x1 = 1;
  int x2 = 2;
  int x3 = 3;
  int x4 = 4;
  int x5 = 5;
  int x6 = 6;

  std::vector<int*> vec = {
    &x0, &x1, &x2, &x3, &x4, &x5, &x6
  };

  GenericTree<int> t;
  t.addNode(x0, NULL);
  t.addNode(x1, &x0);
  t.addNode(x2, &x0);
  t.addNode(x3, &x2);
  t.addNode(x4, &x3);
  t.addNode(x5, &x2);
  // t.print();

  // Load tree from disk

  Diatom d_tree = diatomFromFile("tree.diatom");
  assert(d_tree.is_table());
  assert(d_tree["tree"].is_table());
  assert(d_tree["tree"]["tree"].is_table());
  assert(d_tree["tree"]["free_list"].is_table());
  d_tree["tree"].print();

  GenericTree<int> t_d;
  t_d.fromDiatom(d_tree["tree"], vec);
  t_d.walk([](int *x, int i) {
    printf("%d: %x\n", i, *x);
  });

  t_d.print();
    // This should result in the following result from below:
    //    0 - 1
    //      - 2 - 5       (2 free list entries)

  // Node addition & removal

  t.reset();
  t.print();
    // Should print "Tree is empty"

  t.addNode(x0, NULL);
  t.addNode(x1, &x0);
  t.addNode(x2, &x0);
  t.addNode(x3, &x2);
  t.addNode(x4, &x3);
  t.addNode(x5, &x2);

  t.print();
    // This should result in a tree as follows:
    //    0 - 1
    //      - 2 - 3 - 4
    //          - 5

  t.removeNode(x3, true);
  t.print();
    // This should result in:
    //    0 - 1
    //      - 2 - 5       (2 free list entries)

  Diatom d_reserlzd = t.toDiatom(vec);

  t.addNode(x3, &x5);
  t.print();
    // This should result in:
    //    0 - 1
    //      - 2 - 5 - 3   (1 free list entries)

  t.addNode(x6, NULL);
  t.addNode(x4, &x6);
  t.print();
    // Should rebase the tree:
    //    6 - 0 - 1
    //          - 2 - 5 - 3
    //      - 4              (0 free list entries)

  t.removeNode(x6, true);
  t.print();
    // Tree empty; 7 free list entires

  // printf("%s", serlzd.c_str());

  return 0;
}

