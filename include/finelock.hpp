/**
 * Fine-grained locking implementation, inspired by Herlily paper.
 */

#include "skiplist.h"
#include <thread>
#include <mutex>
#include <bits/stdc++.h>

#ifndef FINELOCK_H
#define FINELOCK_H
template <typename T>
class FineNode {
    public:
    FineNode * volatile *_next;
    T* volatile _value;
    const int _key;
    const int _top_level;
    volatile bool _fully_linked;
    volatile bool _marked;
    std::mutex _lock;
    FineNode(int key, T *value, int top_level) 
        : _value(value), _key(key), _top_level(top_level), _fully_linked(false), _marked(false) {
        _next = new FineNode<T> *[top_level];
    }
    ~FineNode() {
        delete[] _next;
    }
};

template<typename T>
static void unlock(FineNode<T> **preds, int highest_locked) {
    FineNode<T> *pred, *prev_pred = nullptr;
    for (int level = 0; level <= highest_locked; level++) {
        pred = preds[level];
        if(pred != prev_pred) {
            pred->_lock.unlock();
            prev_pred = pred;
        }
    }
}

template <typename T>
class FineLockList : public SkipList<T> {
    private:
    FineNode<T> *_leftmost;
    DeletionManager<FineNode<T>> *_manager;

    int search(int key, FineNode<T> **left_list, FineNode<T> **right_list) {
        FineNode<T> *left = this->_leftmost;
        FineNode<T> *left_next;
        int lFound = -1;
        for(int level = this->_max_level - 1; level >= 0; level--) {
            // begin at most sparse, highway, level
            left_next = left->_next[level]; // curr = pred->_next[layer]

            /* Find unmarked node pair at this level. */
            while (left_next->_key < key) {
                left = left_next;
                left_next = left->_next[level]; //shift forward
            }
            if (lFound == -1 && key == left_next->_key) {
                lFound = level;
            }
            left_list[level] = left;
            right_list[level] = left_next;
        }
        return lFound;
    }

    bool ok_to_delete(FineNode<T> *candidate, int lFound) {
        return (candidate->_fully_linked
            && (candidate->_top_level == lFound+1)
            && (!candidate->_marked));
    }

    public:
    FineLockList(int max_level, double p, int max_deletions=1000) : SkipList<T>(max_level, p) {
        _leftmost = new FineNode<T>(INT_MIN, nullptr, max_level);
        _manager = new DeletionManager<FineNode<T> >(max_deletions);
        FineNode<T> *rightmost = new FineNode<T>(INT_MAX, nullptr, this->_max_level);
        for(int i = 0; i < this->_max_level; i++) {
            _leftmost->_next[i] = rightmost;
            rightmost->_next[i] = nullptr;
        }
    }
    ~FineLockList() override {
        FineNode<T> *curr = _leftmost;
        FineNode<T> *next = curr->_next[0];
        while(next != nullptr) {
            delete curr;
            curr = next;
            next = next->_next[0];
        }
        delete curr;
        delete _manager;
    }

    T *update(int key, T *value) override {
        // TODO update old value too
        assert(value != nullptr); // cannot update with a nullptr (call remove instead)
        assert(key != INT_MIN && key != INT_MAX); // cannot update min and max keys
        int top_level = this->rand_level();
        FineNode<T> *preds[this->_max_level];
        FineNode<T> *succs[this->_max_level];
        while (true) {
            int lFound = search(key, preds, succs);
            if (lFound != -1) {
                FineNode<T> *node_found = succs[lFound];
                if (!node_found->_marked) {
                    while (!node_found->_fully_linked) { /*std::this_thread::yield();*/ } // wait
                    // update value 
                    node_found->_lock.lock();
                    T *old_value = node_found->_value;
                    node_found->_value = value;
                    node_found->_lock.unlock();
                    return old_value; // return previous value
                }
                continue;
            }
            int highest_locked = -1;
            FineNode<T> *pred, *succ, *prev_pred = nullptr;
            bool valid = true;
            for (int level = 0; valid && level < top_level; level++) {
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
                //std::this_thread::yield();
                continue;
            }
            FineNode<T> *new_node = new FineNode<T>(key, value, top_level);
            for (int level = 0; level < top_level; level++) {
                new_node->_next[level] = succs[level];
                preds[level]->_next[level] = new_node;
            }
            new_node->_fully_linked = true;
            unlock(preds, highest_locked);
            return nullptr; // there was no previous value
        }

    }
    T *remove(int key) override {
        assert(key != INT_MIN && key != INT_MAX); // cannot remove min and max keys
        FineNode<T> *node_to_delete = nullptr;
        bool is_marked = false;
        int top_level = -1;
        T* value = nullptr;
        FineNode<T> *preds[this->_max_level], *succs[this->_max_level];
        while (true) {
            int lFound = search(key, preds, succs);
            if (is_marked || 
                (lFound != -1 && ok_to_delete(succs[lFound],lFound))) {
                if (!is_marked) {
                    node_to_delete = succs[lFound];
                    top_level = node_to_delete->_top_level;
                    node_to_delete->_lock.lock();
                    value = node_to_delete->_value;
                    if (node_to_delete->_marked) {
                        // oops! another thread is removing this node
                        node_to_delete->_lock.unlock();
                        return value; // could not delete; returning old value
                    }
                    // continue to delete node
                    node_to_delete->_marked = true;
                    is_marked = true;
                    node_to_delete->_lock.unlock();
                }
                int highest_locked = -1;
                FineNode<T> *pred, *succ, *prev_pred = nullptr;
                bool valid = true;
                for (int level = 0;
                        valid && (level < top_level);
                        level++) {
                    pred = preds[level];
                    succ = succs[level];
                    if (pred != prev_pred) { // lock nodes
                        pred->_lock.lock();
                        highest_locked = level;
                        prev_pred = pred;
                    }
                    valid = (!pred->_marked) && (pred->_next[level] == succ);
                }
                if (!valid) {
                    unlock(preds, highest_locked);
                    //std::this_thread::yield();
                    continue;
                }
                for (int level = top_level-1; level >= 0; level--) {
                    preds[level]->_next[level] = node_to_delete->_next[level];
                }
                unlock(preds, highest_locked);
                _manager->add(node_to_delete);
                return value;
            }
            else return nullptr;
        }
    }
    T *lookup(int key) override {
        FineNode<T> *_[this->_max_level];
        FineNode<T> *succs[this->_max_level];
        int lFound = search(key, _, succs);
        return (lFound != -1 
                && succs[lFound]->_fully_linked 
                && !succs[lFound]->_marked) 
                ? succs[0]->_value : nullptr;
    }

    void print() override {
        std::cout << "Fine-grained locking skip list: ";
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

    bool is_correct() { return true; }
};
#endif