#include <random>
#ifndef SKIPLIST_H
#define SKIPLIST_H

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
    double _p;
    int _max_level;

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
    SkipList(int max_level, double p) : distribution(0.0, 1.0), _p(p), _max_level(max_level) {}

    // available to anyone
    public:
    /**
     * Update a mapping of key -> value, inserting the key if it is not already
     * present. The old value is returned; if the key is not present, nullptr
     * is returned. The argument "value" cannot be equal to nullptr.
     */
    virtual T *update(int key, T* value);
    
    /**
     * Remove a key from the list, return the value associated with the key.
     * If the key is not present in the list, nullptr is returned. 
     */
    virtual T *remove(int key);
    
    /**
     * Returns the value associated with a key. It returns nullptr if it is
     * not present.
     */
    virtual T *lookup(int key);

    /**
     * Prints the list (implemented by subclass).
     */
    virtual void print();

    /**
     * Implemntation performs a check for correctness. 
     */
    virtual bool is_correct();
};
#endif