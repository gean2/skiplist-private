#include <random>
#include <atomic>
#include <assert.h>
#ifndef SKIPLIST_H
#define SKIPLIST_H

/** 
 * Extremely simple class that keeps track of deleted instances of user-defined
 * classes. This has a thread-safe, lock-free method to add to the tracked 
 * deleted nodes, and a thread-unsafe method to free the memory associated with
 * all tracked deleted items. The DeletionManager can be deleted to free all 
 * memory associated with it (this is understandably, not thread-safe).  
*/
template<typename T>
class DeletionManager {
    private:
    T** _deleted;
    std::atomic<int> _deletion_idx; // tracks the next index to point to a given variable
    int _max_deletions; // tracks the maximum number of deletions that this can support
    public:

    /**
     * Instantiate a deletion manager with the maximum number of deletions it
     * can support before it needs to be cleared. 
     */
    DeletionManager(int max_deletions) 
            : _deletion_idx(0), _max_deletions(max_deletions) {
        _deleted = new T *[max_deletions];
    }

    /** 
     * Thread-unsafe method to clear memory associated with the deletion
     * manager. 
     */
    void clear() {
        for(int i = 0; i < _deletion_idx; i++) {
            delete _deleted[i];
        }
        _deletion_idx = 0;
    }

    /** 
     * Thread-unsafe method to delete the deletion manager. 
     */
    ~DeletionManager() {
        clear();
        delete[] _deleted;
    }

    /**
     * Thread-safe operation to add an item to be tracked by the deletion
     * manager.
     */
    void add(T* item) {
        int idx = atomic_fetch_add(&_deletion_idx, 1);
        assert(idx < _max_deletions);
        _deleted[idx] = item;
    }
};

/**
 * This is a header file for skip lists that support unique int keys and
 * T * as the values (templated).
 * 
 * Note that implementations generally only support INT_MIN+1 -> INT_MAX-1 keys.
 */
template <typename T>
class SkipList {
    // private to this class
    private:
    std::default_random_engine generator;
    std::uniform_real_distribution<double> distribution;

    // methods accessible by this and subclasses
    protected:
    const double _p;
    const int _max_level;

    /**
     * Returns a random level less than or equal to the max level of the
     * overall linked list, with exponential bias towards smaller levels.
     */
    int rand_level() {
        int level = 1;
        while(level < _max_level && distribution(generator) < _p) {
            level++;
        } 
        return level;
    }

    /**
     * Abstract constructor for the SkipList. A SkipList should never be instantiated
     * directly.
     */
    SkipList(int max_level, double p) 
        : distribution(0.0, 1.0), _p(p), _max_level(max_level) { 
            assert(max_level > 0 && p >= 0.0 && p <= 1.0);
    }

    // available to anyone
    public:

    virtual ~SkipList() = default;

    /**
     * Update a mapping of key -> value, inserting the key if it is not already
     * present. The old value is returned; if the key is not present, nullptr
     * is returned. The argument "value" cannot be equal to nullptr.
     */
    virtual T *update(int key, T* value) = 0;
    
    /**
     * Remove a key from the list, return the value associated with the key.
     * If the key is not present in the list, nullptr is returned. 
     */
    virtual T *remove(int key) = 0;
    
    /**
     * Returns the value associated with a key. It returns nullptr if it is
     * not present.
     */
    virtual T *lookup(int key) = 0;

    /**
     * Prints the list (implemented by subclass).
     */
    virtual void print() = 0;

    /**
     * Implemntation performs a check for correctness. 
     */
    virtual bool is_correct() = 0; // to be overridden
};
#endif