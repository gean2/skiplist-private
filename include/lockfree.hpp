/**
 * Lock-free implementation of compare-and-swap
 */

#include "skiplist.h"
#include <atomic>
#include <bits/stdc++.h>
#include <iostream>

#ifndef LOCK_FREE_H
#define LOCK_FREE_H

#define CAS(obj, expected, desired) atomic_compare_exchange_weak(&obj, &expected, desired)

template<typename T> 
class LockFreeNode{
    public:
    std::atomic<LockFreeNode *>*_next;
    std::atomic<T *>_value;
    const int _key;
    const int _top_level;
    LockFreeNode(int key, T *value, int top_level) 
            : _value(value), _key(key), _top_level(top_level) {
        _next = new std::atomic<LockFreeNode<T> *>[top_level];
    }
    ~LockFreeNode() {
        delete[] _next;
    }
    void mark_node_ptrs() {
        LockFreeNode<T> *x_next;
        for(int i = _top_level-1; i >= 0; i--) {
            do {
                x_next = _next[i].load();
                if(is_marked(x_next)) break;
            } while(!CAS(_next[i], x_next, mark(x_next)));
        }
    }
};

template<typename T>
static bool inline is_marked(LockFreeNode<T> *p) {
    return static_cast<bool>(reinterpret_cast<long>(p) & 0x1L);
}

template<typename T>
static LockFreeNode<T> inline *unmark(LockFreeNode<T> *p) {
    return reinterpret_cast<LockFreeNode<T> *>(reinterpret_cast<long>(p) & ~0x1L);
}

template<typename T>
static LockFreeNode<T> inline *mark(LockFreeNode<T> *p) {
    return reinterpret_cast<LockFreeNode<T> *>(reinterpret_cast<long>(p) | 0x1L);
}

template <typename T>
class LockFreeList : public SkipList<T> {
    private:
    LockFreeNode<T> *_leftmost; // header, etc.
    DeletionManager<LockFreeNode<T> > *_manager;

    void search(int key, LockFreeNode<T> **left_list, LockFreeNode<T> **right_list) {
        retry: LockFreeNode<T> *left = _leftmost;
        LockFreeNode<T> *left_next;
        LockFreeNode<T> *right;
        LockFreeNode<T> *right_next;
        for(int i = this->_max_level - 1; i >= 0; i--) {
            left_next = left->_next[i].load();
            if(is_marked(left_next)) {
                goto retry;
            }
            /* Find unmarked node pair at this level. */
            for(right = left_next; ; right = right_next) {
                /* Skip a sequence of marked nodes. */
                while(true) {
                    right_next = right->_next[i].load();
                    if(!is_marked(right_next)) break;
                    right = unmark(right_next);
                }
                if(right == nullptr || right->_key >= key) break;
                left = right; left_next = right_next;
            }
            /* Ensure left and right nodes are adjacent. */
            if((left_next != right) && !CAS(left->_next[i], left_next, right)) {
                goto retry;
            }
            left_list[i] = left; right_list[i] = right;
        }
    }

    public:
    LockFreeList(int max_level, double p, int max_deletions=1000) 
            : SkipList<T>(max_level, p) {
        _leftmost = new LockFreeNode<T>(INT_MIN, nullptr, max_level);
        _manager = new DeletionManager<LockFreeNode<T> >(max_deletions);
        LockFreeNode<T> *rightmost = new LockFreeNode<T>(INT_MAX, nullptr, this->_max_level);
        for(int i = 0; i < this->_max_level; i++) {
            _leftmost->_next[i] = rightmost;
            rightmost->_next[i] = nullptr;
        }
    }

    /**
     * NOT THREAD-SAFE. To be called to destroy all memory associated with the
     * linked list (including deleted nodes that have not been freed yet)
     */
    ~LockFreeList() override {
        LockFreeNode<T> *curr = _leftmost;
        LockFreeNode<T> *next = curr->_next[0].load();
        while(next != nullptr) {
            delete curr;
            curr = next;
            next = next->_next[0].load();
        }
        delete curr;
        delete _manager;
    }

    T *update(int key, T *value) override {
        assert(value != nullptr); // cannot update with a nullptr (call remove instead)
        assert(key != INT_MIN && key != INT_MAX); // cannot update min and max keys
        LockFreeNode<T> *node = new LockFreeNode<T>(key, value, this->rand_level());
        LockFreeNode<T> *preds[this->_max_level];
        LockFreeNode<T> *succs[this->_max_level];
        retry: search(key, preds, succs);
        /* Update the value field of an existing node. */
        if(succs[0]->_key == key) {
            T *old_value;
            do {
                old_value = succs[0]->_value.load();
                if(old_value == nullptr) {
                    succs[0]->mark_node_ptrs();
                    goto retry;
                }
            } while (!CAS(succs[0]->_value, old_value, value));
            delete node; // do not need this newly created node
            return old_value;
        }
        for(int i = 0; i < node->_top_level; i++) node->_next[i] = succs[i];
        /* Node is visible once inserted at lowest level. */
        if(!CAS(preds[0]->_next[0], succs[0], node)) goto retry;
        for(int i = 1; i < node->_top_level; i++) {
            while (true) {
                LockFreeNode<T> *pred = preds[i];
                LockFreeNode<T> *succ = succs[i];
                /* Update the forward pointer if it is stale. */
                LockFreeNode<T> *new_next = node->_next[i].load();
                LockFreeNode<T> *unmarked = unmark(new_next);
                if ((new_next != succ) && (!CAS(node->_next[i], unmarked, succ))) {
                    break; /* Give up if pointer is marked. */
                }
                /* Check for old reference to a ‘k’-node. */
                if(succ->_key == key) succ = unmark(succ->_next[i].load());
                /* We retry the search if the CAS fails. */
                if(CAS(pred->_next[i], succ, node)) break;
                search(key, preds, succs);
            }
        }
        return nullptr; /* No existing mapping was replaced. */
    }

    T *remove(int key) override {
        assert(key != INT_MIN && key != INT_MAX); // cannot remove min and max keys
        LockFreeNode<T> *_[this->_max_level];
        LockFreeNode<T> *succs[this->_max_level];
        search(key, _, succs);
        if(succs[0]->_key != key) return nullptr; // key is not in list
        T *value;
        /* 1. Node is logically deleted when the value field is set to nullptr */
        do {
            value = succs[0]->_value.load();
            if(value == nullptr) return nullptr;
        } while(!CAS(succs[0]->_value, value, static_cast<T *>(nullptr)));
        /* 2. Mark forward pointers, then search will remove the node. */
        LockFreeNode<T> *to_delete = succs[0];
        to_delete->mark_node_ptrs();
        search(key, _, succs);
        // "delete" node by keeping a reference to it in an array
        _manager->add(to_delete);
        return value;
    }

    T *lookup(int key) override {
        LockFreeNode<T> *_[this->_max_level];
        LockFreeNode<T> *succs[this->_max_level];
        search(key, _, succs);
        return (succs[0]->_key == key) ? succs[0]->_value.load() : nullptr;
    }

    void print() override {
        std::cout << "Lock free skip list: ";
        for(int i = this->_max_level-1; i >= 0; i--) {
            LockFreeNode<T> *curr = _leftmost;
            std::cout << "L" << i << ": ";
            while(curr->_next[i].load() != nullptr) {
                std::cout << curr->_key << ",";
                curr = curr->_next[i].load();
            }
            std::cout << curr->_key << "; ";
        }
        std::cout << "\n";
    }

    /** 
     * Thread unsafe method to be called when threads have finished reading,
     * writing, etc. to deleted nodes, in order to free the memory associated
     * with deleted nodes.
     */
    void cleanup() {
        _manager->clear();
    }

    bool is_correct() { return true; }
};
#endif