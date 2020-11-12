//
// GenericTree_Referential.h
//
// A version of GenericTree which holds value types, and copies them into its vector,
// instead of pointers in an externally-managed array.
//
// Note while revisiting when converting mercurial -> git:
//  - the more important difference is this identifies items by index
//  - should be built so one can store either value types or pointers
//  - then could either require the user to track index or not
//
// MIT licensed: http://opensource.org/licenses/MIT
//

#ifndef __GenericTree_Referential_h
#define __GenericTree_Referential_h

#include <vector>
#include <cstdio>

// Optionally enable serialization
#ifdef _GTR_ENABLE_SERIALIZATION
  #include "Diatom.h"
  #define _GTR_SZ(x) x
#else
  #define _GTR_SZ(x)
#endif

// Optionally disable asserts
#ifdef _GTR_DISABLE_SAFETY_CHECKS
  #define _assert(x)
#else
  #include <cassert>
  #define _assert(x) assert(x)
#endif

#define __GTR_NOT_FOUND -1


template <class T>
class GenericTree_Referential {
public:

  struct NodeInfo {
    T node;
    int index_of_parent;
    std::vector<int> children;
  };

  void reset() {
    nodes.clear();
    free_list.clear();
  }

  int addNode(T x, int parent_ind = __GTR_NOT_FOUND) {
    NodeInfo ni = { x, parent_ind };

    bool was_empty = isEmpty();


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
    if (parent_ind != __GTR_NOT_FOUND)
      nodes[ni.index_of_parent].children.push_back(ind);

    // If the node is being inserted at the top, deal with current
    // top node (if present)
    else if (!was_empty) {
      int i_top = indexOfTopNode();
      if (i_top != __GTR_NOT_FOUND) {
        nodes[ind].children.push_back(i_top);
        nodes[i_top].index_of_parent = ind;
      }
    }

    return ind;
  }

  void removeNode(int i, bool recursivelyRemoveChildren) {
    NodeInfo &node = nodes[i];
    free_list.push_back(i);

    int i_parent = node.index_of_parent;
    if (i_parent != __GTR_NOT_FOUND) {
      removeChild_ForNodeAtIndex(i_parent, i);
    }

    if (recursivelyRemoveChildren) {
      removeChildren(i);
    }
  }

  void print() {
    int i_top = indexOfTopNode();
    if (i_top == __GTR_NOT_FOUND) {
      printf("[Tree is empty]\n");
    }
    else {
      walk([](T n, int i, int indent) {
        for (int i=0; i < indent; ++i) {
          if (i == indent - 1)
            printf("└──");
          else printf("   ");
        }

        printf("☐  ");
        _print(n);
        printf("    index: %d \n", i);
      });
    }

    size_t n__free_list = free_list.size();
    printf("%lu %s on free list", n__free_list, n__free_list == 1 ? "entry" : "entries");
    if (n__free_list > 0) {
      printf(" - ");
      for (int &i : free_list) {
        printf("%d ", i);
      }
    }
    printf("\n\n");
  }

  template <class Functor>
  void walk(Functor f, int i = -2, int indent = 0) {
    if (i == -2) i = indexOfTopNode();
    if (i == __GTR_NOT_FOUND) return;

    f(nodes[i].node, i, indent);

    for (auto &i : nodes[i].children) {
      walk(f, i, indent + 1);
    }
  }

  template <class Functor, class Return>
  void walk_and_pass(Functor f, const Return &r_parent, int i = -2) {
    if (i == -2) i = indexOfTopNode();
    if (i == __GTR_NOT_FOUND) return;

    Return r = f(nodes[i].node, r_parent, i);
    for (auto &i : nodes[i].children)
      walk_and_pass(f, r, i);
  }
    // walk_and_pass:
    // Walk, passing the return value of the functor to all of the element's children.

  int indexOfTopNode() {
    if (isEmpty())
      return __GTR_NOT_FOUND;

    int i_top = 0;
    while (indexIsInFreeList(i_top))
      ++i_top;

    while (nodes[i_top].index_of_parent != __GTR_NOT_FOUND)
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
    _assert(node_i < nodes.size());
    return nodes[node_i].children;
  }

  int childOfNode(int node_i, int child_i) {
    _assert(node_i < nodes.size());
    _assert(child_i < nodes[node_i].children.size());
    return nodes[node_i].children[child_i];
  }

  T get(int i) {
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

  bool indexIsInFreeList(int ind) {
    for (const auto &i : free_list)
      if (i == ind) return true;
    return false;
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

_GTR_SZ(

  /*** Serialization ***/

  // In order to serialize, the nodes themselves must be serializable:
  //    _toDiatom() and _fromDiatom() functions must be defined for the
  //    node type, before GenericTree_Referential is #included.

public:

  Diatom toDiatom() {
    Diatom d;

    // Tree
    {
      d["tree"] = Diatom();

      int i = 0;
      for (auto n : nodes) {
        Diatom &d_node = d["tree"][srlz_index(i++)] = Diatom();

        d_node["node"] = _toDiatom(n.node);
        d_node["parent_ind"] = (double) n.index_of_parent;
        Diatom &dch = d_node["child_inds"] = Diatom();
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

  void fromDiatom(Diatom &d) {
    _assert(d.is_table());
    reset();

    Diatom &d_tree      = d["tree"];
    Diatom &d_free_list = d["free_list"];
    _assert(d_tree.is_table());
    _assert(d_free_list.is_table());

    // Tree
    d_tree.each([&](std::string &key, Diatom &dn) {
      Diatom &d_node       = dn["node"];
      Diatom &d_parent_ind = dn["parent_ind"];
      Diatom &d_child_inds = dn["child_inds"];

      _assert(d_parent_ind.is_number());
      _assert(d_child_inds.is_table());

      typename GenericTree_Referential<T>::NodeInfo n;
      n.node            = _fromDiatom(d_node);
      n.index_of_parent = (int) d_parent_ind.number_value();

      d_child_inds.each([&](std::string &ch_key, Diatom &dc) {
        _assert(dc.is_number());
        n.children.push_back((int) dc.number_value());
      });

      nodes.push_back(n);
    });

    // Free list
    d_free_list.each([&](std::string, Diatom &li) {
      _assert(li.is_number());
      free_list.push_back((int) li.number_value());

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

) // _GTR_SZ

};


#endif  // ifndef __GenericTree_Referential_h

