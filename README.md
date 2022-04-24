# skiplist public readme
15-418/618 Final Project: Concurrent Skip Lists


## Project Proposal

### Summary

We will implement three versions of concurrent skip lists using C/C++ and manual memory management: coarse-grained locking skip list, fine-grained locking skip list, and a lock-free skip list. 

### Background

A skip list is a probabilistic data structure of sorted elements that on average offers O(log n) search complexity and O(log n) insertion complexity. (At worst, it offers O(n) complexity for both, but this is extremely unlikely.) A skip list consists of a hierarchy of (logically) individual linked lists, which can be considered layers. "Upper" layers are subsets of the "lower" layers at each level, and each successively "lower" layer skips over fewer and fewer elements. The lowest layer contains all the elements in sorted order, i.e. no skipped elements.

During lookup, we iterate through the "upper" layers up until we see that the next element is greater than the desired key, at which point we go down to the next "lower" layer. We repeat this process until we reach the lowest layer which contains all elements, where we then find or don't find the desired item. During insertion, we follow the same lookup process (while keeping track of previous nodes in each layer). When we find the node we will be inserting after, we pick a random height (exponentially biased), and change the links in the new node and the node previous. During deletion, we similarly look up the desired node, change the pointers in each layer to skip over the deleted node, and then free the memory associated with the node.

Skip lists are often used in distributed applications as well as the underlying implementation for concurrent priority queues. In these applications, performance for concurrent operations is a major concern. Correctness of a thread-safe skip list can easily be ensured through a coarse-grained locking scheme, i.e. lock whenever an insertion, removal, or search is performed. Reader-writer locks can be used to slightly improve performance by allowing searches to occur simultaneously, but this lock scheme does not allow for insertions and removals to be done concurrently. In contrast, a finer-grained locking scheme can allow concurrent non-conflicting insertions, removals, and searches by locking only the two nodes whose pointers are updated by any given writing operation.

A concurrent lock-free skip list allows insertions, removals, and searches to be run simultaneously through careful usage of CAS semantics to ensure correctness. This is done through pointer marking. When a node is being deleted, the first step (after finding the node to be deleted) is to "mark" its pointers to indicate that nodes should not be inserted after the node. After this operation, preceding pointers can be changed, etc. Furthermore, a reference counter must be used for each node to ensure that the memory allocated to a node is not freed while any other thread has access to it. 

### Challenges

A significant challenge is achieving a successful, correct implementation of the lock-free skip list. We need to ensure that when insertions and deletions are happening concurrently, no nodes are being added after already deleted nodes (which would cause these nodes to be inaccessible as they point to the now deleted node). Furthermore, we need to ensure that an inserted node is fully linked so that lookups of the linked list have a correct view of linked list. Of special concern is ensuring that the memory for deleted nodes is not freed while other nodes have access to them, but the memory allocated for deleted nodes is eventually freed (to prevent memory leakage). 

Similarly, for the fine-grained locking implementation, we need to ensure correctness as above, and we also need to ensure that deadlocks never occur.  This means that we need to make sure that we take all locks in the same order. Finally, we need to ensure that we drop all locks and retry whenever we are unable to grab a lock as needed. 

### Resources

We will use the GHC machines to implement and test our skip list implementations. We will use the PSC machines to measure runtime of our benchmarking code.

We use the following papers to guide our implementations of the skip lists:

1. Fraser, Keir. *Practical lock-freedom*. No. UCAM-CL-TR-579. University of Cambridge, Computer Laboratory, 2004.
2. William Pugh. Concurrent Maintenance of Skip Lists. Technical Report CS-TR-2222, Department of Computer Science, University of Maryland, June 1990
3. Herlihy, Maurice, et al. "A provably correct scalable concurrent skip list." *Conference On Principles of Distributed Systems (OPODIS). Citeseer*. 2006.

### Goals and Deliverables

1. Provide implementations of a coarse-grained locking, fine-grained locking, and lock-free skip list
2. Measured runtime and speedup across different distributions of work requested, looking at how our data structure performs with mostly insertions, mostly deletions, or mostly searches. We will be comparing the runtime across these different workloads as well as different skip list implementations and number of threads (up to 128 threads on the PSC machines).
3. Discussion and analysis of the reasons behind speedup trends between different skip list implementations and also why speedup is not perfect, etc.

We will most likely implement Herlihy's purportedly "simpler" fine-grained locking skip list implementation; if we have extra time, we may implement two separate fine-grained locking implementations to compare the performance of the two.

If we encounter issues with our project, we will not implement any of the fine-grained locking skip lists, and only compare implementations of the coarse-grained locking implementation and Fraser's lock-free skip list instead.

### Platform Choice

We will largely be using C++ to implement skip lists because this provides us fine-grained synchronization control as well as control over memory allocation and freeing, which is not provided by other programming languages. C++ has an advantage over C in that it has some nice libraries (such as "random") and convenient object-oriented programming support, which simplifies test code. We will need to use C to implement the CAS operations, and we will call these functions through C++ header files.

### Timeline

4/20: Write new project proposal; finish coarse-grained locking skip list (mostly done; just needs to be made more generalizable)

4/21-4/22: Implement lock-free skip list lookup and insert (with testing code)

4/23-4/26: Implement lock-free skip list remove (with testing code)

4/27-4/29: Debug lock-free skip list if necessary; otherwise implement fine-grained locking skip list

4/30-5/3: Write benchmark code; execute benchmarking code on PSC machines

5/3-5/5: Write project report 