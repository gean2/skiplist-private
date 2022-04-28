#include "include/finelock.h"
#include <limits.h>
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
    : _value(value), _key(key), _top_level(top_level) {
    _next = new FineNode<T> *[top_level];
}

template<typename T>
FineNode<T>::~FineNode() {
    delete[] _next;
}

/**
 * Mark all nodes that rely on this node (e.g. if highway under construction,
 * gonna mess up residential road traffic too)
*/
template<typename T>
void FineNode<T>::mark_node_ptrs() {
    // FineNode<T> *x_next;
    for(int i = _top_level-1; i >= 0; i--) {
        _next[i] = mark(_next[i]);
    }
}

template<typename T>
void FineList<T>::unlock(FineNode<T> *preds, int highest_locked) {
    FineNode<T> *curr;
    for (int i = highest_locked; i >= 0; i--) {
        std::cout << "TODO";
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
    FineNode<T> *curr = this->_leftmost;
    FineNode<T> *next = curr->_next[0];
    while(next != nullptr) {
        delete curr;
        curr = next;
        next = next->_next[0];
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
int FineList<T>::search(int key, FineNode<T> **left_list, 
                                 FineNode<T> **right_list, T**value) {
    FineNode<T> *left = this->_leftmost;
    FineNode<T> *left_next;
    int lFound = -1;
    for(int level = this->_max_level - 1; level >= 0; level--) {
        // begin at most sparse, highway, level
        left_next = left->_next[level]; // curr = pred->_next[layer]

        /* Find unmarked node pair at this level. */
        while (left->_key < key) {
            left_next = left;
            left = left_next->_next[level]; // TODO comment
        }
        if (lFound == -1 && key == left->_key) {
            lFound = level;
            *value = left->_value;
        }
        left_list[level] = left;
        right_list[level] = left_next;
    }
    return lFound;
}

template<typename T>
T *FineList<T>::lookup(int key) {
    FineNode<T> *_[this->_max_level];
    FineNode<T> *succs[this->_max_level];
    T* value;
    search(key, _, succs, &value);
    return (succs[0]->_key == key) ? succs[0]->_value : nullptr;
}

template<typename T>
bool FineList<T>::ok_to_delete(FineNode<T> *candidate, int lFound) {
    return (candidate->fully_linked
            && candidate->_top_level == lFound
            && !candidate->_marked);
}

template<typename T>
bool FineList<T>::contains(int v) {
    FineNode<T> *preds[this->_max_level], *succs[this->_max_level];
    T *val;
    int lFound = search(v, preds, succs, &val);
    return (lFound != -1
            && succs[lFound]->_fully_linked
            && !succs[lFound]->_marked);
}

template<typename T>
T *FineList<T>::remove(int key) {
    assert(key != INT_MIN && key != INT_MAX); // cannot remove min and max keys
    FineNode<T> *node_to_delete = nullptr;
    bool is_marked = false;
    int top_level = -1;
    FineNode<T> *preds[this->_max_level], *succs[this->_max_level];
    while (true) {
        T *ret = nullptr;
        int lFound = search(key, preds, succs, &ret);
        if (is_marked || 
            (lFound != -1 && ok_to_delete(succs[lFound],lFound))
            ) {
            if (!is_marked) {
                node_to_delete = succs[lFound];
                top_level = node_to_delete->_top_level;
                node_to_delete->_lock.lock();
                if (node_to_delete->_marked) {
                    // oops! another thread is removing this node
                    node_to_delete->_lock.unlock();
                    return nullptr; // could not delete
                }
                node_to_delete->_marked = true;
                is_marked = true;
            }
            int highest_locked = -1;
            FineNode<T> *pred, *succ, *prev_pred = nullptr;
            bool valid = true;
            for (int level = 0;
                    valid && (level <= top_level);
                    level++) {
                pred = preds[level];
                succ = succs[level];
                if (pred != prev_pred) {
                    pred->_lock.lock();
                    highest_locked = level;
                    prev_pred = pred;
                }
                valid = !pred->_marked && pred->_next[level] == succ;
            }
            if (!valid) {
                unlock(preds, highest_locked);
                continue;
            }
            for (int level = top_level; level >= 0; level--) {
                preds[level]->_next[level] = node_to_delete->_next[level];
            }
            node_to_delete->_lock.unlock();
            unlock(preds, highest_locked);
            return ret;
        }
        else return nullptr;
    }


    FineNode<T> *_[this->_max_level];
    // where the hell did i get this code?
    // FineNode<T> *succs[this->_max_level];
    search(key, _, succs);
    if(succs[0]->_key != key) return nullptr; // key is not in list
    T *value = succs[0]->value;
    /* 1. Node is logically deleted when the value field is set to nullptr */
    succs[0]->value = nullptr;
    /* 2. Mark forward pointers, then search will remove the node. */
    FineNode<T> *to_delete = succs[0];
    to_delete->mark_node_ptrs();
    search(key, _, succs);
    delete to_delete;
    return value;
}

template<typename T>
bool FineList<T>::add(int key, T *value) {
    // TODO update old value too
    assert(value != nullptr); // cannot update with a nullptr (call remove instead)
    assert(key != INT_MIN && key != INT_MAX); // cannot update min and max keys
    int ran_level = this->rand_level();
    FineNode<T> *node = new FineNode<T>(key, value, ran_level);
    FineNode<T> *preds[this->_max_level];
    FineNode<T> *succs[this->_max_level];
    search(key, preds, succs);
    /* Update the value field of an existing node. */
    while (true) {
        int lFound = search(key, preds, succs);
        if (lFound != -1) {
            auto node_found = succs[lFound];
            if (!node_found->_fully_linked) {
                while (!node_found->_fully_linked) {} // wait
                return false;
            }
            continue;
        }
        int highest_locked = -1;
        FineNode<T> *pred, *succ, *prev_pred = nullptr;
        bool valid = true;
        for (int level = 0; valid && level <= ran_level; level++) {
            pred = preds[level];
            succ = succs[level];
            if (pred != prev_pred) {
                // only lock where needed
                pred->_lock.lock();
                highest_locked = level;
                prev_pred = pred;
            }
            valid = !pred->_marked && !succ->_marked
                    && pred->_next[level] == succ;
        }
        if (!valid) {
            unlock(preds, highest_locked);
            continue;
        }
        auto new_node = new FineNode<T> *(key, value, ran_level);
        for (int level = 0; level <= ran_level; level++) {
            new_node->_next[level] = succs[level];
            preds[level]->_next[level] = new_node;
        }
        new_node->_fully_linked = true;
        unlock(preds, highest_locked);
        return true;
    }
}

template<typename T>
void FineList<T>::print() {
    std::cout << "Fine-grain locking skip list: ";
    for(int i = this->_max_level-1; i >= 0; i--) {
        FineNode<T> *curr = _leftmost;
        std::cout << "L" << i << ": ";
        while(curr->_next[i] != nullptr) {
            std::cout << curr->_key << ",";
            curr = curr->_next[i];
        }
        std::cout << curr->_key << "; ";
    }
    std::cout << "\n";
}