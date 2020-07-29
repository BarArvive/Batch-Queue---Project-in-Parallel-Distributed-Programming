//
// Created by BAR on 22/04/2020.
//

#ifndef BQ_BATCHREQUEST_H
#define BQ_BATCHREQUEST_H


#include <atomic>


/*
* A trivial node structure for creating linked list,
* except using atomic pointer to the next, allowing use atomic CAS.
*/
template<typename T>
class Node {
public:
    T *item;
    std::atomic<Node<T> *> next;
};


/*
* A BatchRequest is prepared by a thread that initiates a batch,
* and consists of the details of the batch’s pending operations:
* firstEnq and lastEnq are pointers to the first and last nodes
* of a linked list containing the pending items to be enqueued; 
* enqsNum, deqsNum, and excessDeqsNum are, respectively, the numbers of enqueues,
* dequeues and excess dequeues in the batch.
*/
template<typename T>
class BatchRequest {
public:
    Node<T> *firstEnq;
    Node<T> *lastEnq;
    unsigned int enqsNum;
    unsigned int deqsNum;
    unsigned int excessDeqsNum;
};


#endif //BQ_BATCHREQUEST_H
