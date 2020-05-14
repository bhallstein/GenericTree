//
// GenericTree.h
//
// A tree.
//
// Parent & child relationships are tracked using indices.
//
// Serialization:
//  - you can serialize the tree by writing out all tracked indices
//  - i.e. convert the node pointers to indices in an (external) object array, then just write out
//    the nodes array and the free list
//
// MIT licensed: http://opensource.org/licenses/MIT

#ifndef __GenericTree_h
#define __GenericTree_h

#include <vector>
#include <cstdio>

// Optionally enable serialization
#ifdef _GT_ENABLE_SERIALIZATION
  #include "Diatom.h"
  #define _GT_SZ(x) x
#else
  #define _GT_SZ(x)
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

  void print() {
    int i_top = indexOfTopNode();
    if (i_top == __GT_NOT_FOUND)
      printf("[Tree is empty]\n");
    else
      recursivelyPrintNode(i_top);

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
    if (i == -2) i = indexOfTopNode();
    if (i == __GT_NOT_FOUND) return;

    f(nodes[i].node, i);

    for (auto &i : nodes[i].children)
      walk(f, i);
  }

  int indexOfTopNode() {
    if (isEmpty())
      return __GT_NOT_FOUND;

    int i_top = 0;
    while (indexIsInFreeList(i_top))
      ++i_top;

    while (nodes[i_top].index_of_parent != __GT_NOT_FOUND)
      i_top = nodes[i_top].index_of_parent;
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
    int ind = indexOfNode(x);
    if (ind != __GT_NOT_FOUND) return !indexIsInFreeList(ind);
    return false;
  }
  bool indexIsInFreeList(int ind) {
    for (const auto &i : free_list)
      if (i == ind) return true;
    return false;
  }

  int indexOfNode(T &x) {
    for (int i=0, n = int(nodes.size()); i < n;  ++i)
      if (nodes[i].node == &x)
        return i;
    return __GT_NOT_FOUND;
  }

  void removeChild_ForNodeAtIndex(int parent_ind, int child_ind) {
    _assert(parent_ind < nodes.size());
    _assert(child_ind < nodes.size());

    NodeInfo &parent_node = nodes[parent_ind];
    std::vector<int> &children = parent_node.children;
    std::vector<int>::iterator it;

    for (it = children.begin(); it != children.end(); ++it) {
      if (*it == child_ind)
        break;
    }

    _assert(it != children.end());
    children.erase(it);
  }

  int indentCount = 0;
  void recursivelyPrintNode(int i) {
    for (int i=0; i < indentCount; ++i) {
      if (i == indentCount - 1)
        printf("└──");
      else printf("   ");
    }

    NodeInfo &n = nodes[i];

    printf("☐  %d  index: %d  ", *n.node, i);
    printf("children: ");
    for (auto &i : n.children)
      printf("%d ", i);
    printf(" parent: ");
    if (n.index_of_parent == __GT_NOT_FOUND) printf("[none]");
    else printf("%d", n.index_of_parent);
    printf("\n");

    indentCount += 1;
    for (auto &i : nodes[i].children)
      recursivelyPrintNode(i);
    indentCount -= 1;
  }

_GT_SZ(

  /*** Serialization ***/

  // Assuming the tree owner has a vector of pointers to the nodes in the tree,
  // we can easily serialize the tree by writing out all the relevant indices.

  int indexOfOriginalNodeInVector(T *node, std::vector<T*> &vec) {
    for (int i=0, n = (int)vec.size(); i < n; ++i)
      if (vec[i] == node)
        return i;
    return __GT_NOT_FOUND;
  }

public:

  Diatom toDiatom(std::vector<T*> &original_nodes) {
    Diatom d;

    // Tree
    {
      d["tree"] = Diatom();

      int i = 0;
      for (auto n : nodes) {
        int node_orig_ind = indexOfOriginalNodeInVector(n.node, original_nodes);
        _assert(node_orig_ind != __GT_NOT_FOUND);

        Diatom &d_node = d["tree"][srlz_index(i++)] = Diatom();

        d_node["node_orig_ind"] = (double) node_orig_ind;
        d_node["parent_gt_ind"] = (double) n.index_of_parent;
        Diatom &dch = d_node["child_gt_inds"] = Diatom();
        int j = 0;
        for (auto ind : n.children)
          dch[srlz_index(j++)] = (double) ind;
      }
    }

    // Free list
    {
      d["free_list"] = Diatom();
      int i = 0;
      for (auto ind : free_list)
        d["free_list"][srlz_index(i++)] = (double) ind;
    }

    return d;
  }

  void fromDiatom(Diatom &d, std::vector<T*> &ext_nodes) {
    _assert(d.is_table());
    reset();

    Diatom &d_tree      = d["tree"];
    Diatom &d_free_list = d["free_list"];
    _assert(d_tree.is_table());
    _assert(d_free_list.is_table());

    // Tree
    d_tree.each([&](std::string &key, Diatom &dn) {
      Diatom &d_node_orig_ind = dn["node_orig_ind"];
      Diatom &d_parent_gt_ind = dn["parent_gt_ind"];
      Diatom &d_child_gt_inds = dn["child_gt_inds"];

      _assert(d_node_orig_ind.is_number());
      _assert(d_parent_gt_ind.is_number());
      _assert(d_child_gt_inds.is_table());

      int node_orig_ind = (int) d_node_orig_ind.value__number;
      _assert(node_orig_ind >= 0 && node_orig_ind < ext_nodes.size());

      typename GenericTree<T>::NodeInfo n;
        n.node            = ext_nodes[node_orig_ind];
        n.index_of_parent = (int) d_parent_gt_ind.value__number;

      d_child_gt_inds.each([&](std::string &ch_key, Diatom &dc) {
        _assert(dc.is_number());
        n.children.push_back((int) dc.value__number);
      });

      nodes.push_back(n);
    });

    // Free list
    d_free_list.each([&](std::string &key, Diatom &li) {
      _assert(li.is_number());
      free_list.push_back((int) li.value__number);

      // NOTE: Suppose the free list contains an index beyond the 'used' portion
      //  of the nodes vector. In this case after deserialization conceivably we could
      //  crash when adding a node using that index.
      //       However, this shouldn't be an issue since we serialize/deserialize the
      //  entire nodes vector - even items that are on the free list. (Supposing we're
      //  wrong in thinking it's impossible, there is an assert in addNode() to catch this
      //  eventuality.)
    });
  }

private:
  static std::string srlz_index(int i) {
    return std::string("n") + std::to_string(i);
  }

) // _GT_SZ

};


#endif  // ifndef __GenericTree_h

