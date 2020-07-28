//
// Created by BAR on 22/04/2020.
//

#ifndef BQ_THREADDATE_H
#define BQ_THREADDATE_H

#include "BatchRequest.h"
#include "FutureOp.h"
#include <queue>

using std::queue;


/*
* A threadData array holds local data for each thread.
* First, the pending operations details are kept, in the order they were called,
* in an operation queue opsQueue, implemented as a simple local non-thread-safe queue.
* It contains FutureOp items. Second, the items of the pending enqueue operations
* are kept in a linked list in the order they were enqueued by FutureEnqueue calls.
* This list is referenced by enqsHead and enqsTail (with no dummy nodes here).
* Lastly, each thread keeps record of the number of FutureEnqueue and FutureDequeue
* operations that have been called but not yet applied, and the number of excess dequeues.
*/
template<typename T>
class ThreadData {
public:
    queue<FutureOp<T>> opsQueue;
    Node<T> *enqsHead;
    Node<T> *enqsTail;
    unsigned int enqsNum;
    unsigned int deqsNum;
    unsigned int excessDeqsNum;
    bool warning;
    Node<T>* nodeHp;
    Ann<T>* annHp;
};


#endif //BQ_THREADDATE_H
