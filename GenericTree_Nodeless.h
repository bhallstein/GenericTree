//
// GenericTree.h
//
// A tree.
//
// Parent & child relationships are tracked using indices.
//
// Serialization:
//  - you can serialize the tree by writing out all tracked indices
//  - i.e. convert the node pointers to indices in an (external) object array, then
//    just write out the nodes array and the free list
//
// MIT licensed: http://opensource.org/licenses/MIT

#ifndef __GenericTree_h
#define __GenericTree_h

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

#define __GT_NOT_FOUND -1


template <class T>
class GenericTree {
public:
  struct NodeInfo {
    T *node;
    int index_of_parent;
    std::vector<int> children;
  };

  void reset() {
    nodes.clear();
    free_list.clear();
  }

  int addNode(T &x, T *parent) {
    _assert(!nodeIsPresent(x));
    NodeInfo ni = { &x, __GT_NOT_FOUND };


    bool was_empty = isEmpty();

    if (parent) {
      ni.index_of_parent = indexOfNode(*parent);
      _assert(ni.index_of_parent != __GT_NOT_FOUND);
    }


    // Add the node to the nodes vector
    int ind = -1;

    if (free_list.size() > 0) {
      ind = free_list.back();
      free_list.pop_back();
      _assert(ind < nodes.size());
      nodes[ind] = ni;
    }

    else {
      nodes.push_back(ni);
      ind = int(nodes.size()) - 1;
    }

    // Add node to parent's children array
    if (parent)
      nodes[ni.index_of_parent].children.push_back(ind);

    // If the node is being inserted at the top, deal with current
    // top node (if present)
    else if (!was_empty) {
      int i_top = indexOfTopNode();
      if (i_top != __GT_NOT_FOUND) {
        nodes[ind].children.push_back(i_top);
        nodes[i_top].index_of_parent = ind;
      }
    }

    return ind;
  }

  void removeNode(T &x, bool recursivelyRemoveChildren) {
    int i = indexOfNode(x);
    _assert(i != __GT_NOT_FOUND);

    NodeInfo &node = nodes[i];
    free_list.push_back(i);

    int parent_ind = node.index_of_parent;
    if (parent_ind != __GT_NOT_FOUND) {
      removeChild_ForNodeAtIndex(parent_ind, i);
    }

    if (recursivelyRemoveChildren)
      removeChildren(i);
  }

  void flat_print() {
    for (int i=0; i < nodes.size(); ++i) {
      printf("i: %d\n", i);

      bool skip = indexIsInFreeList(i);
      if (skip) {
        printf("- in free list\n");
      }

      if (!skip) {
        printf("- index_of_parent: %d\n", nodes[i].index_of_parent);
        printf("- children:\n");
        for (auto c : nodes[i].children) {
          printf("  - %d\n", c);
        }
      }
    }
  }

  void print() {
    int i_top = indexOfTopNode();
    if (i_top == __GT_NOT_FOUND) {
      printf("[Tree is empty]\n");
    }
    else {
      recursivelyPrintNode(i_top);
    }

    size_t n_fl = free_list.size();
    printf("%lu %s on free list", n_fl, n_fl == 1 ? "entry" : "entries");
    if (n_fl > 0) {
      printf(" - ");
      for (int &i : free_list)
        printf("%d ", i);
    }
    printf("\n\n");
  }

  template <class Functor>
  void walk(Functor f, int i = -2) {
    if (i == -2) {
      i = indexOfTopNode();
    }

    if (i == __GT_NOT_FOUND) {
      return;
    }

    auto &n = nodes[i];

    f(n.node, i);
    for (auto &i : n.children) {
      walk(f, i);
    }
  }

  int indexOfTopNode() {
    if (isEmpty()) {
      return __GT_NOT_FOUND;
    }

    int i_top = 0;
    while (indexIsInFreeList(i_top)) {
      ++i_top;
    }

    while (nodes[i_top].index_of_parent != __GT_NOT_FOUND) {
      i_top = nodes[i_top].index_of_parent;
    }

    return i_top;
  }

  int parentOfNode(int i) {
    _assert(i < nodes.size());
    return nodes[i].index_of_parent;
  }

  int nChildren(int node_i) {
    _assert(node_i < nodes.size());
    return (int) nodes[node_i].children.size();
  }
  std::vector<int> children(int node_i) {
    assert(node_i < nodes.size());
    return nodes[node_i].children;
  }

  int childOfNode(int node_i, int child_i) {
    _assert(node_i < nodes.size());
    _assert(child_i < nodes[node_i].children.size());
    return nodes[node_i].children[child_i];
  }

  T* get(int i) {
    _assert(i < nodes.size());
    return nodes[i].node;
  }

  bool isEmpty() {
    return (nodes.size() == free_list.size());
  }

protected:
  std::vector<NodeInfo> nodes;
  std::vector<int> free_list;

  void removeChildren(int i) {
    _assert(i < nodes.size());

    for (auto &i : nodes[i].children) {
      removeChildren(i);
      free_list.push_back(i);
    }
  }

  bool nodeIsPresent(T &x) {
    // The node is present if it is in the nodes array, and its index is not in
    // the free list
    int i = indexOfNode(x);
    return i != __GT_NOT_FOUND && !indexIsInFreeList(i);
  }

  bool indexIsInFreeList(int ind) {
    for (const auto &i : free_list) {
      if (i == ind) {
        return true;
      }
    }
    return false;
  }

  int indexOfNode(T &x) {
    for (int i=0, n = int(nodes.size()); i < n;  ++i) {
      if (nodes[i].node == &x) {
        return i;
      }
    }
    return __GT_NOT_FOUND;
  }

  void removeChild_ForNodeAtIndex(int parent_ind, int child_ind) {
    _assert(parent_ind < nodes.size());
    _assert(child_ind < nodes.size());

    NodeInfo &parent_node = nodes[parent_ind];
    std::vector<int> &children = parent_node.children;
    std::vector<int>::iterator it;

    for (it = children.begin(); it != children.end(); ++it) {
      if (*it == child_ind) {
        break;
      }
    }

    _assert(it != children.end());
    children.erase(it);
  }

  int indentCount = 0;
  void recursivelyPrintNode(int i) {
    for (int i=0; i < indentCount; ++i) {
      if (i == indentCount - 1) { printf("└──"); }
      else                      { printf("   "); }
    }

    NodeInfo &n = nodes[i];

    printf("☐  index: %d  ", i);
    printf("children: ");
    for (auto &i : n.children) {
      printf("%d ", i);
    }
    printf(" parent: ");
    if (n.index_of_parent == __GT_NOT_FOUND) { printf("[none]"); }
    else                                     { printf("%d", n.index_of_parent); }
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

  Diatom toDiatom(std::vector<T*> &original_nodes) {
    Diatom d;

    // Tree
    {
      d["tree"] = Diatom();

      int i = 0;
      for (auto n : nodes) {
        int i__ext = indexOfOriginalNodeInVector(n.node, original_nodes);
        _assert(i__ext != __GT_NOT_FOUND);

        Diatom &d_node = d["tree"][srlz_index(i++)] = Diatom();

        d_node["i__ext"] = (double) i__ext;
        d_node["i__parent"] = (double) n.index_of_parent;
        Diatom &dch = d_node["i__children"] = Diatom();
        int j = 0;
        for (auto ind : n.children) {
          dch[srlz_index(j++)] = (double) ind;
        }
      }
    }

    // Free list
    {
      d["free_list"] = Diatom();
      int i = 0;
      for (auto ind : free_list) {
        d["free_list"][srlz_index(i++)] = (double) ind;
      }
    }

    return d;
  }

  void fromDiatom(Diatom &d, std::vector<T*> &ext_nodes) {
    _assert(d.is_table());
    _assert(d["tree"].is_table());
    _assert(d["free_list"].is_table());

    reset();

    // Tree
    d["tree"].each([&](std::string &key, Diatom &dnode) {
      _assert(dnode["i__ext"].is_number());
      _assert(dnode["i__parent"].is_number());
      _assert(dnode["i__children"].is_table());

      int i__ext = (int) dnode["i__ext"].value__number;
      _assert(i__ext >= 0 && i__ext < ext_nodes.size());

      GenericTree<T>::NodeInfo n;
      n.node            = ext_nodes[i__ext];
      n.index_of_parent = (int) dnode["i__parent"].value__number;

      dnode["i__children"].each([&](std::string &ch_key, Diatom &dc) {
        _assert(dc.is_number());
        n.children.push_back((int) dc.value__number);
      });

      nodes.push_back(n);
    });

    // Free list
    d["free_list"].each([&](std::string &key, Diatom &li) {
      _assert(li.is_number());
      free_list.push_back((int) li.value__number);

      // NOTE: Suppose the free list contains an index beyond the 'used' portion
      //  of the nodes vector. In this case after deserialization conceivably we could
      //  crash when adding a node using that index.
      //       However, this shouldn't be an issue since we serialize/deserialize the
      //  entire nodes vector - even items that are on the free list. (Supposing we're
      //  wrong in thinking it's impossible, an assert in addNode() checks for this.)
    });
  }

protected:
  int indexOfOriginalNodeInVector(T *node, std::vector<T*> &vec) {
    for (int i=0, n = (int)vec.size(); i < n; ++i) {
      if (vec[i] == node) {
        return i;
      }
    }
    return __GT_NOT_FOUND;
  }

private:
  static std::string srlz_index(int i) {
    return std::string("n") + std::to_string(i);
  }

#endif

};


#endif  // ifndef __GenericTree_h

