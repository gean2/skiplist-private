#include "include/lockfree.h"
#include <bits/stdc++.h>
#include <atomic>
#include <iostream>
#include <mutex>

/*
template <typename T>
class FineNode {
    public:
    FineNode **_next;
    T *_value;
    int _key;
    int _top_level;
    bool _fully_linked;
    Lock lock; // TODO what is this
    FineNode(int key, T *value, int top_level);
    ~FineNode();
};
*/

#define CAS(obj, expected, desired) atomic_compare_exchange_weak(&obj, &expected, desired)

// TODO wait do I mark for fine grain?

/**
 * we use the lowest bit in the pointer field to represent if the node is 
 * marked, to allow operations like add and remove to seem atomic
*/
template<typename T>
bool inline is_marked(FineNode<T> *p) {
    return static_cast<bool>(reinterpret_cast<long>(p) & 0x1L);
}

template<typename T>
FineNode<T> inline *unmark(FineNode<T> *p) {
    return reinterpret_cast<FineNode<T> *>(reinterpret_cast<long>(p) & ~0x1L);
}

/**
 * assuming list nodes are word-aligned, unmarked nodes will have 0 at the 
 * lowest order bit, so an unmarked node being marked will only have that bit
 * changed
*/
template<typename T>
FineNode<T> inline *mark(FineNode<T> *p) {
    return reinterpret_cast<FineNode<T> *>(reinterpret_cast<long>(p) | 0x1L);
}

/**
 * Initialize a fine grain locking node
*/
template<typename T>
FineNode<T>::FineNode(int key, T *value, int top_level)
    : _value(value), _key(key), _top_level(top_level), _refcount(0) {
    _next = new std::atomic<FineNode<T> *>[top_level];
}

template<typename T>
FineNode<T>::~FineNode() {
    int i = 0;
    while(!CAS(this->_refcount, i, 0)) {} // only free memory after refcount is zero
    delete[] _next;
}

/**
 * Mark all nodes that rely on this node (e.g. if highway under construction,
 * gonna mess up residential road traffic too)
*/
template<typename T>
void FineNode<T>::mark_node_ptrs() {
    FineNode<T> *x_next;
    for(int i = _top_level-1; i >= 0; i--) {
        do {
            x_next = _next[i].load();
            if(is_marked(x_next)) break;
        } while(!CAS(_next[i], x_next, mark(x_next)));
    }
}

/**
 * initialize list: TODO
*/
template <typename T>
FineList<T>::FineList(int max_level, double p) : SkipList<T>(max_level, p) {
    _leftmost = new FineNode<T>(INT_MIN, nullptr, max_level);
    FineNode<T> *rightmost = new FineNode<T>(INT_MAX, nullptr, this->_max_level);
    for(int i = 0; i < this->_max_level; i++) {
        _leftmost->_next[i] = rightmost;
        rightmost->_next[i] = nullptr;
    }
}

/**
 * destructor for FineList
*/
template <typename T>
FineList<T>::~FineList() {
    FineNode<T> *curr = _leftmost;
    FineNode<T> *next = curr->_next[0].load();
    while(next != nullptr) {
        delete curr;
        curr = next;
        next = next->_next[0].load();
    }
    delete curr;
}

/**
 * Equivalent to FindNode in the herlily paper, but keeping search for sake
 * of consistency
 * left_list: ___
 * right_list: ___
 * TODO: should return type be key type or value type?
*/
template<typename T>
int::search(int key, FineNode<T> **left_list, FineNode<T> **right_list) {
    retry: FineNode<T> *left = _leftmost;
    FineNode<T> *left_next;
    int lFound = -1;
    for(int level = this->_max_level - 1; level >= 0; level--) {
        // begin at most sparse, highway, level
        left_next = left->_next[level].load(); // curr = pred->nexts[layer]
        // if(is_marked(left_next)) {
        //     // oops, this level for this node is under construction
        //     // try again (and hopefully then it's been taken care of)
        //     goto retry;
        // }
        /* Find unmarked node pair at this level. */
        while (left->key < v) {
            left_next = left;
            left = left_next->_next[level]; // TODO comment
        }
        if (lFound == -1 && v == left->_key) {
            lFound = level;
        }
        left_list[level] = left;
        right_list[level] = left_next;
    }
}

template<typename T>
T *FineList<T>::lookup(int key) {
    FineNode<T> *_[this->_max_level];
    FineNode<T> *succs[this->_max_level];
    search(key, _, succs);
    return (succs[0]->_key == key) ? succs[0]->_value.load() : nullptr;
}

bool ok_to_delete(FineNode<T> *candidate, int lFound) {
    return (candidate->fully_linked
            && candidate->_top_level == lFound
            && !candidate->_marked);
}

bool contains(int v) {
    FineNode<T> *preds[this->_max_height], *succs[this->_max_height];
    int lFound = search(v, preds, succs);
    return (lFound != -1
            && succs[lFound]->_fully_linked
            && !succs[lFound]->_marked);
}

template<typename T>
T *FineList<T>::remove(int key) {
    assert(key != INT_MIN && key != INT_MAX); // cannot remove min and max keys
    FineNode<T> *node_to_delete = null;
    bool is_marked = false;
    int top_level = -1;
    FineNode<T> *preds[this->_max_height], *succs[this->_max_height];
    while (true) {
        int lFound = find_node(key, preds, succs);
        if (is_marked || 
            (lFound != -1 && ok_to_delete(succs[lFound],lFound))
            ) {
            if (!is_marked) {
                node_to_delete = succs[lFound];
                top_level = node_to_delete->_top_level;
                node_to_delete->lock.lock();
                if (node_to_delete->_marked) {
                    // oops! another thread is removing this node
                    node_to_delete->lock.unlock();
                    return false; // could not delete
                }
                node_to_delete->_marked = true;
                is_marked = true;
            }
            int highest_locked = -1;
            try {
                FineNode<T> *pred, *succ, *prev_pred = null;
                bool valid = true;
                for (int level = 0;
                     valid && (level <= top_level)
                     level++) {
                    pred = preds[level];
                    succ = succs[level];
                    if (pred != prev_pred) {
                        pred->lock.lock();
                        highest_locked = level;
                        prev_pred = pred;
                    }
                    valid = !pred->_marked && pred->nexts[level] == succ;
                }
                if (!valid) continue;
                for (int level = top_level; level >= 0; level--) {
                    preds[level]->_next[level] = node_to_delete->_next[level];
                }
                node_to_delete->lock.unlock();
                return true;
            } finally {
                unlock(preds, highest_locked);
            }
        }
        else return false;
    }


    FineNode<T> *_[this->_max_level];
    FineNode<T> *succs[this->_max_level];
    search(key, _, succs);
    if(succs[0]->_key != key) return nullptr; // key is not in list
    T *value;
    /* 1. Node is logically deleted when the value field is set to nullptr */
    do {
        value = succs[0]->_value.load();
        if(value == nullptr) return nullptr;
    } while(!CAS(succs[0]->_value, value, static_cast<T *>(nullptr)));
    /* 2. Mark forward pointers, then search will remove the node. */
    FineNode<T> *to_delete = succs[0];
    to_delete->mark_node_ptrs();
    search(key, _, succs);
    delete to_delete;
    return value;
}

template<typename T>
T *FineList<T>::add(int key, T *value) {
    // TODO update old value too
    assert(value != nullptr); // cannot update with a nullptr (call remove instead)
    assert(key != INT_MIN && key != INT_MAX); // cannot update min and max keys
    int ran_level = this->rand_level();
    FineNode<T> *node = new FineNode<T>(key, value, ran_level);
    FineNode<T> *preds[this->_max_level];
    FineNode<T> *succs[this->_max_level];
    retry: search(key, preds, succs);
    /* Update the value field of an existing node. */
    while (true) {
        int lFound = search(key, preds, succs);
        if (lFound != -1) {
            auto node_found = succs[lFound];
            if (!node_found->_fully_linked) {
                while (!nodeFound->_fully_linked) {} // wait
                return false;
            }
        }
        continue;
    }
    int highest_locked = -1;
    try {
        FineNode<T> *pred, *succ, *prev_pred = null;
        bool valid = true;
        for (int level = 0; valid && level <= ran_level; level++) {
            pred = preds[level];
            succ = succs[level];
            if (pred != prev_pred) {
                // only lock where needed
                pred->lock.lock();
                highest_locked = llevel;
                prev_pred = -red;
            }
            valid = !pred->_marked && !succ->_marked
                    && pred->nexts[level] == succ;
        }
        if (!valid) continue;
        auto new_node = new Node(v, ran_level);
        for (int level = 0; level <= ran_level; level++) {
            new_node->nexts[level] = succs[level];
            preds[level]->nexts[level] = new_node;
        }
        new_node->_fully_linked = true;
        return true;
    }
    finally { 
        unlock(preds, highest_locked);
    }

}

template<typename T>
void FineList<T>::print() {
    std::cout << "Lock free skip list: ";
    for(int i = this->_max_level-1; i >= 0; i--) {
        FineNode<T> *curr = _leftmost;
        std::cout << "L" << i << ": ";
        while(curr->_next[i].load() != nullptr) {
            std::cout << curr->_key << ",";
            curr = curr->_next[i].load();
        }
        std::cout << curr->_key << "; ";
    }
    std::cout << "\n";
}