//
// Created by BAR on 22/04/2020.
//

#ifndef BQ_FUTUREOP_H
#define BQ_FUTUREOP_H


/*
* A Future contains a result, which holds the return value of the deferred operation
* that generated the future (for dequeues only, as enqueue operations have no return value)
* and an isDone Boolean value, which is true only if the deferred computation has been completed.
* When isDone is false, the contents of result may be arbitrary.
*/
template <typename T>
class Future {
public:
    T* result = NULL;
    bool isDone = false;
};

/*
* The type of the future operation - ENQ for enqueue, DEQ for dequeue.
*/
enum Type{ENQ, DEQ};

/*
* This struct note a future operation for the queue, Enqueue or Dequeue. 
* When calling future operation, an FutureOp is created, and added to a 
* local queue, for executing it later.
*/
template <typename T>
class FutureOp {
public:
    Future<T>* future;
    Type type;
};


#endif //BQ_FUTUREOP_H
