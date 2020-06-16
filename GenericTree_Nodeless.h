//
// GenericTree_Nodeless.h
//
// - This version of GenericTree just tracks indices
// - Is probably better to use this than GenericTree
//
// MIT licensed: http://opensource.org/licenses/MIT
//

#ifndef __GenericTree_Nodeless_h
#define __GenericTree_Nodeless_h

#include <vector>
#include <cstdio>

// Optionally enable serialization
#ifdef _GT_ENABLE_SERIALIZATION
  #include "Diatom/Diatom.h"
#endif

// Optionally disable asserts
#ifdef _GT_DISABLE_SAFETY_CHECKS
  #define _assert(x)
#else
  #include <cassert>
  #define _assert(x) assert(x)
#endif


class GenericTree_Nodeless {
public:
  struct Node {
    int parent;
    std::vector<int> children;
  };

  void reset() {
    nodes.clear();
    free_list.clear();
  }

  int addNode(int parent) {
    Node node = { parent };

    bool was_empty = isEmpty();

    // Return an index for the caller to use to add the node to the externally-managed vector.
    int i = -1;
    int i__prev_top = was_empty ? -1 : indexOfTopNode();

    if (free_list.size() > 0) {
      i = free_list.back();
      free_list.pop_back();

      nodes[i] = node;
    }
    else {
      i = (int) nodes.size();
      nodes.push_back(node);
    }

    // Add node to parent's children array
    if (parent != -1) {
      nodes[parent].children.push_back(i);
    }

    // Or if the node is being inserted at the top, deal with current
    // top node (if present)
    if (parent == -1 && !was_empty) {
      nodes[i].children.push_back(i__prev_top);
      nodes[i__prev_top].parent = i;
    }

    return i;
  }

  template<class T>
  int addNodeAndInsert(int parent, T item, std::vector<T> &ext_nodes) {
    int i = addNode(parent);

    if (i == ext_nodes.size()) {
      ext_nodes.push_back(item);
    }
    else {
      ext_nodes[i] = item;
    }

    return i;
  }

  void removeNode(int i, bool recursively_remove_children) {
    _assert(i > 0 && i < nodes.size());

    Node &node = nodes[i];
    free_list.push_back(i);

    if (node.parent != -1) {
      unmakeChild(node.parent, i);
    }

    if (recursively_remove_children) {
      removeChildren(i);
    }
  }

  int indexForChild(int parent, int child_in_children_vec) {
    _assert(parent >= 0 && parent < nodes.size());
    _assert(child_in_children_vec >= 0);
    _assert(child_in_children_vec < nodes[parent].children.size());

    return nodes[parent].children[child_in_children_vec];
  }

  void print() {
    int i_top = indexOfTopNode();
    if (i_top == -1) {
      printf("[Tree is empty]\n");
    }
    else {
      recursivelyPrintNode(i_top);
    }

    size_t n_fl = free_list.size();
    printf("%lu %s on free list", n_fl, n_fl == 1 ? "entry" : "entries");
    if (n_fl > 0) {
      printf(" - ");
      for (int &i : free_list) {
        printf("%d ", i);
      }
    }
    printf("\n\n");
  }

  template <class Functor>
  void walk(Functor f, int i = -2) {
    if (i == -1) {
      return;
    }

    if (i == -2) {
      i = indexOfTopNode();
    }

    f(i);
    for (auto &i : nodes[i].children) {
      walk(f, i);
    }
  }

  int indexOfTopNode() {
    if (isEmpty()) {
      return -1;
    }

    int i_top = 0;
    while (indexIsInFreeList(i_top)) {
      ++i_top;
    }
    while (nodes[i_top].parent != -1) {
      i_top = nodes[i_top].parent;
    }

    return i_top;
  }

  int nChildren(int i) {
    return (int) nodes[i].children.size();
  }

  int parentIndex(int i) {
    return nodes[i].parent;
  }

  bool isEmpty() {
    return nodes.size() == free_list.size();
  }

protected:
  std::vector<Node> nodes;
  std::vector<int> free_list;

  void removeChildren(int i) {
    _assert(i < nodes.size());

    for (auto &i : nodes[i].children) {
      removeChildren(i);
      free_list.push_back(i);
    }
  }

  bool nodeIsPresent(int i) {
    return i != -1 && !indexIsInFreeList(i);
  }

  bool indexIsInFreeList(int i) {
    for (const auto &f : free_list) {
      if (i == f) {
        return true;
      }
    }
    return false;
  }

  void unmakeChild(int parent, int child_to_remove) {
    _assert(parent > 0 && parent < nodes.size());
    _assert(child_to_remove > 0 && child_to_remove < nodes.size());

    auto &children = nodes[parent].children;

    std::vector<int>::iterator it;
    for (it = children.begin(); it != children.end(); ++it) {
      if (*it == child_to_remove) {
        break;
      }
    }

    if (it != children.end()) {
      children.erase(it);
    }
  }

  int indentCount = 0;
  void recursivelyPrintNode(int i) {
    for (int i=0; i < indentCount; ++i) {
      if (i == indentCount - 1) { printf("└──"); }
      else                      { printf("   "); }
    }

    Node &n = nodes[i];

    printf("☐  index: %d  ", i);
    printf("children: ");
    for (auto &i : n.children) {
      printf("%d ", i);
    }
    printf(" parent: ");
    if (n.parent == -1) { printf("[none]"); }
    else                { printf("%d", n.parent); }
    printf("\n");

    indentCount += 1;
    for (auto &i : nodes[i].children) {
      recursivelyPrintNode(i);
    }
    indentCount -= 1;
  }

#ifdef _GT_ENABLE_SERIALIZATION

public:

  // Serialization
  // -----------------------------
  // Assuming the tree owner has a vector of pointers to the nodes in the tree,
  // we can serialize the tree by writing out all the relevant indices.

  Diatom toDiatom() {
    Diatom d;

    // Tree
    {
      d["tree"] = Diatom();

      for (int i=0; i < nodes.size(); ++i) {
        Node &n = nodes[i];
        Diatom &d_node = d["tree"][srlz_index(i)] = Diatom();

        d_node["i"] = (double) i;
        d_node["i__parent"] = (double) n.parent;
        d_node["i__children"] = Diatom();
        for (int j=0; j < n.children.size(); ++j) {
          d_node["i__children"][srlz_index(j)] = (double) n.children[j];
        }
      }
    }

    // Free list
    {
      d["free_list"] = Diatom();
      for (int i=0; i < free_list.size(); ++i) {
        d["free_list"][srlz_index(i)] = (double) i;
      }
    }

    return d;
  }

  void fromDiatom(Diatom &d) {
    _assert(d.is_table());
    _assert(d["tree"].is_table());
    _assert(d["free_list"].is_table());

    reset();

    // Tree
    d["tree"].each([&](std::string &key, Diatom &item) {
      _assert(item["i"].is_number());
      _assert(item["i__parent"].is_number());
      _assert(item["i__children"].is_table());

      int i = (int) item["i"].value__number;
      int parent = (int) item["i__parent"].value__number;

      Node n = { parent };

      item["i__children"].each([&](std::string &ch_key, Diatom &c) {
        _assert(c.is_number());
        n.children.push_back((int) c.value__number);
      });

      if (nodes.size() <= i) {
        nodes.resize(i + 1);
      }
      nodes[i] = n;
    });

    // Free list
    d["free_list"].each([&](std::string &key, Diatom &f) {
      _assert(f.is_number());
      free_list.push_back((int) f.value__number);

      // NOTE: Suppose the free list contains an index beyond the 'used' portion
      //  of the nodes vector. In this case after deserialization conceivably we could
      //  crash when adding a node using that index.
      //       However, this shouldn't be an issue since we serialize/deserialize the
      //  entire nodes vector - even items that are on the free list. (Supposing we're
      //  wrong in thinking it's impossible, an assert in addNode() checks for this.)
    });
  }

private:
  static std::string srlz_index(int i) {
    return std::string("n") + std::to_string(i);
  }

#endif

};


#endif  // ifndef __GenericTree_h

