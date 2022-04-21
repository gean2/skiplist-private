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
    int rand_level() {
        int level = 1;
        while(level < _max_level && distribution(generator) < _p) {
            level++;
        } 
        return level;
    }
    int max_level() { return _max_level; }

    // available to anyone
    public:
    SkipList(int max_level, double p) : distribution(0.0, 1.0), _p(p), _max_level(max_level) {} 
    virtual bool insert(int key, T* value);
    virtual T *remove(int key);
    virtual T *lookup(int key);
    virtual void print();
};
#endif