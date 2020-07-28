//
// Created by BAR on 22/04/2020.
//

#ifndef BQ_QUEUE_H
#define BQ_QUEUE_H

#include "PtrCntOrAnn.h"
#include "BatchRequest.h"
#include "FutureOp.h"
#include "ThreadData.h"
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <memory>

#define NUM_OF_DEL 1000000

// struct for three differnt types and items.
template<typename T1, typename T2, typename T3>
class Three {
public:
    T1 item1;
    T2 item2;
    T3 item3;

    Three(T1 item1, T2 item2, T3 item3) {
        this->item1 = item1;
        this->item2 = item2;
        this->item3 = item3;
    }
};


/*
* simple memory management, which delete only data which is not used anymore
*/
template<typename T>
class Mem {
public:
    std::allocator <Node<T>> NodeAlloc;
    std::allocator <Ann<T>> AnnAlloc;

    Node<T> *AllocateNode() {
        return NodeAlloc.allocate(1);
    }

    Ann<T> *AllocateAnn() {
        return AnnAlloc.allocate(1);
    }

    void Retire(Node<T> *node, std::queue<Node<T> *> *node_queue, ThreadData<T> *threadData) {
        node_queue->push(node);
        if (node_queue->size() > NUM_OF_DEL) {
            Node<T> *nodel = node_queue->front();
            node_queue->pop();
            NodeAlloc.deallocate(nodel, 1);
        }
    }

    void Free(Node<T> *node) {
        NodeAlloc.deallocate(node, 1);
    }

    void Retire(Ann<T> *ann) {
        AnnAlloc.deallocate(ann, 1);
    }

    void Free(Ann<T> *ann) {
        AnnAlloc.deallocate(ann, 1);
    }

};

/*
* Similarly to MSQ, the shared queue is represented as a linked list of nodes in which
* the first node is a dummy node, and all nodes thereafter contain the values in the queue
* in the order they were enqueued. We maintain pointers to the first and last nodes
* of this list, denoted ASQHead and ASQTail respectively
* (which stand for Atomic Shared Queue’s Head and Tail).
* Batch operations require the size of the queue for a fast calculation
* of the new head after applying the batch’s operations.
* To this end, Queue maintains counters of the number of enqueues and
* the number of successful dequeues applied so far.
* These are kept size-by-side with the tail and head pointers respectively,
* and are updated atomically with the respective pointers using a double-width CAS.
* This is implemented using a Pointer and Count object (PtrCnt , that can be atomically modified)
* for the head and tail of the shared queue.
*/
template<typename T>
class Queue {
public:
    std::atomic <PtrCntOrAnn<T>> ASQHead;
    std::atomic <PtrCnt<T>> ASQTail;
    unsigned int numOfThreads;
    ThreadData<T> *threadData;
    std::queue<Future<T> *> *futureToDel;
    std::queue<Node<T> *> *nodesToDel;
    std::queue<Ann<T> *> *annsToDel;
    Mem<T> MM;

    /*
    * initializing data: number of threads, queues and arrays.
    * also, creates the first node (dummy node) and point the head
    * and the tail to this node.
    */
    Queue(unsigned int NumOfThreads) {
        numOfThreads = NumOfThreads;
        threadData = new ThreadData<T>[numOfThreads];
        futureToDel = new std::queue<Future<T> *>[numOfThreads];
        nodesToDel = new std::queue<Node<T> *>[numOfThreads];
        annsToDel = new std::queue<Ann<T> *>[numOfThreads];
        Node<T> *newNode = MM.AllocateNode();
        newNode->item = NULL;
        newNode->next = NULL;
        ASQTail.store({newNode, 0}, std::memory_order_relaxed);
        ASQHead.store({{newNode, 0}}, std::memory_order_relaxed);
        for (int k = 0; k < numOfThreads; k++) {
            threadData[k].enqsHead = NULL;
            threadData[k].enqsTail = NULL;
            threadData[k].excessDeqsNum = 0;
            threadData[k].deqsNum = 0;
            threadData[k].enqsNum = 0;
            threadData[k].warning = false;
            threadData[k].nodeHp = NULL;
            threadData[k].annHp = NULL;
        }
    }

    /*
    * EnqueueToShared appends an item after the tail of the shared queue, using
    * two CAS operations.
    */
    void EnqueueToShared(T *item, unsigned int threadId) {
        Node<T> *newNode = MM.AllocateNode();
        newNode->item = item;
        newNode->next = NULL;
        Node<T> *NullNode = NULL;
        PtrCnt<T> tailAndCnt;
        PtrCnt<T> tailAndCntBackup;
        PtrCntOrAnn<T> head;
        while (true) {
            tailAndCntBackup = ASQTail.load(std::memory_order_relaxed);
            tailAndCnt = tailAndCntBackup;
            threadData[threadId].nodeHp = tailAndCnt.node;
            //_memoryFence
            std::atomic_thread_fence(std::memory_order_release);
            if (threadData[threadId].warning) {
                threadData[threadId].warning = false;
                threadData[threadId].nodeHp = NULL;
                continue;
            }
            if (tailAndCnt.node->next.compare_exchange_weak(NullNode, newNode, std::memory_order_release,
                                                            std::memory_order_relaxed)) {
                ASQTail.compare_exchange_weak(tailAndCnt, {newNode, tailAndCnt.cnt + 1}, std::memory_order_release,
                                              std::memory_order_relaxed);
                tailAndCnt = tailAndCntBackup;
                break;
            }
            NullNode = NULL;
            head = ASQHead.load(std::memory_order_relaxed);
            if ((head.annAndTag.tag & 1) != 0) {
                threadData[threadId].nodeHp = NULL;
                threadData[threadId].annHp = head.annAndTag.ann;
                //_memoryFence
                std::atomic_thread_fence(std::memory_order_release);
                if (threadData[threadId].warning) {
                    threadData[threadId].warning = false;
                    threadData[threadId].annHp = NULL;
                    continue;
                }
                ExecuteAnn(head.annAndTag.ann, threadId);
                threadData[threadId].annHp = NULL;
            } else {
                ASQTail.compare_exchange_weak(tailAndCnt, {tailAndCnt.node->next, tailAndCnt.cnt + 1},
                                              std::memory_order_release, std::memory_order_relaxed);
                tailAndCnt = tailAndCntBackup;
                threadData[threadId].nodeHp = NULL;
            }
        }
        threadData[threadId].nodeHp = NULL;
    }

    /*
    * DequeueFromShared extracts an item from the head of the shared queue and returns it, if queue is not empty.
    * Otherwise it returns NULL.
    */
    T *DequeueFromShared(unsigned int thread_id) {
        PtrCnt<T> headAndCnt;
        PtrCntOrAnn<T> headAndCntOrAnn;
        Node<T> *headNextNode;
        T *dequeuedItem;
        while (true) {
            headAndCnt = HelpAnnAndGetHead(thread_id);
            threadData[thread_id].nodeHp = headAndCnt.node;
            //_memoryFence
            std::atomic_thread_fence(std::memory_order_release);
            if (threadData[thread_id].warning) {
                threadData[thread_id].warning = false;
                goto cleanup;
            }
            headAndCntOrAnn = {headAndCnt};
            headNextNode = headAndCnt.node->next;
            if (headNextNode == NULL) {
                threadData[thread_id].nodeHp = NULL;
                return NULL;
            }
            dequeuedItem = headNextNode->item;
            if (ASQHead.compare_exchange_weak(headAndCntOrAnn, {{headNextNode, headAndCnt.cnt + 1}},
                                              std::memory_order_release, std::memory_order_relaxed)) {
                MM.Retire(headAndCnt.node, &nodesToDel[thread_id], threadData);
                return dequeuedItem;
            }

            cleanup:
            threadData[thread_id].nodeHp = NULL;
        }
    }

    /*
    * This auxiliary method assists announcements in execution, as long as
    * there is an announcement installed in ASQHead.
    */
    PtrCnt<T> HelpAnnAndGetHead(unsigned int thread_id) {
        PtrCntOrAnn<T> head;
        while (true) {
            head = ASQHead.load(std::memory_order_relaxed);
            if ((head.annAndTag.tag & 1) == 0)
                return head.ptrCnt;
            threadData[thread_id].annHp = head.annAndTag.ann;
            //_memoryFence
            std::atomic_thread_fence(std::memory_order_release);
            if (threadData[thread_id].warning) {
                threadData[thread_id].warning = false;
                threadData[thread_id].annHp = NULL;
                continue;
            }
            ExecuteAnn(head.annAndTag.ann, thread_id);
            threadData[thread_id].annHp = NULL;
        }
    }

    /*
    * ExecuteBatch is responsible for executing the batch.
    * Before it starts doing so, it checks whether there is a colliding ongoing
    * batch operation whose announcement is installed in ASQHead.
    * If so, ExecuteBatch helps it complete.
    * Afterwards, it stores the current head in ann,
    * installs ann in ASQHead and calls ExecuteAnn to carry out the batch.
    */
    Node<T> *ExecuteBatch(BatchRequest<T> batchRequest, unsigned int thread_id) {
        Ann<T> *ann = MM.AllocateAnn();
        ann->batchReq = batchRequest;
        ann->oldHead = {NULL, 0};
        ann->oldTail = {NULL, 0};
        PtrCnt<T> oldHeadAndCnt;
        PtrCntOrAnn<T> oldHeadAndCntOrAnn;
        PtrCntOrAnn<T> anAnn = {.annAndTag = {1, ann}};
        while (true) {
            oldHeadAndCnt = HelpAnnAndGetHead(thread_id);
            threadData[thread_id].nodeHp = oldHeadAndCnt.node;
            //_memoryFence
            std::atomic_thread_fence(std::memory_order_release);
            if (threadData[thread_id].warning) {
                threadData[thread_id].warning = false;
                goto cleanup;
            }
            oldHeadAndCntOrAnn = {oldHeadAndCnt};
            ann->oldHead = oldHeadAndCnt;
            if (ASQHead.compare_exchange_weak(oldHeadAndCntOrAnn, anAnn, std::memory_order_release,
                                              std::memory_order_relaxed))
                break;
            cleanup:
            threadData[thread_id].nodeHp = NULL;
        }
        threadData[thread_id].nodeHp = NULL;
        ExecuteAnn(ann, thread_id);
        annsToDel[thread_id].push(ann);
        return oldHeadAndCnt.node;

    }

    /*
    * ExecuteAnn is called with ann after ann has been installed in ASQHead.
    * ann’s oldHead field consists of the value of ASQHead right before ann’s installation.
    * ExecuteAnn carries out anns’s batch. If any of the execution steps has already been
    * executed by another thread, ExecuteAnn moves on to the next step.
    * Specifically, if ann will have been removed from SQHead by the time ExecuteAnn is executed,
    * ann’s execution will have been completed,
    * and all the steps of this run of ExecuteAnn would fail and have no effect.
    */
    void ExecuteAnn(Ann<T> *ann, unsigned int thread_id) {
        PtrCnt<T> tailAndCnt;
        PtrCnt<T> annOldTailAndCnt;
        Node<T> *NullNode = NULL;
        while (true) {
            tailAndCnt = ASQTail.load(std::memory_order_relaxed);
            annOldTailAndCnt = ann->oldTail;
            if (annOldTailAndCnt.node != NULL) {
                threadData[thread_id].nodeHp = annOldTailAndCnt.node;
                //_memoryFence
                std::atomic_thread_fence(std::memory_order_release);
                if (threadData[thread_id].warning) {
                    threadData[thread_id].warning = false;
                    if (ASQHead.load(std::memory_order_relaxed).annAndTag.ann != ann) {
                        threadData[thread_id].nodeHp = NULL;
                        return;
                    }
                }
                break;
            }
            threadData[thread_id].nodeHp = tailAndCnt.node;
            //_memoryFence
            std::atomic_thread_fence(std::memory_order_release);
            if (threadData[thread_id].warning) {
                threadData[thread_id].warning = false;
                if (ASQHead.load(std::memory_order_relaxed).annAndTag.ann != ann) {
                    threadData[thread_id].nodeHp = NULL;
                    return;
                }
            }
            Node<T> *frstEnq = ann->batchReq.firstEnq;
            tailAndCnt.node->next.compare_exchange_weak(NullNode, ann->batchReq.firstEnq, std::memory_order_release,
                                                        std::memory_order_relaxed);
            NullNode = NULL;
            ann->batchReq.firstEnq = frstEnq;
            if (tailAndCnt.node->next == ann->batchReq.firstEnq) {
                ann->oldTail = annOldTailAndCnt = tailAndCnt;
                //_memoryFence
                std::atomic_thread_fence(std::memory_order_release);
                break;
            } else {
                ASQTail.compare_exchange_weak(tailAndCnt, {tailAndCnt.node->next, tailAndCnt.cnt + 1},
                                              std::memory_order_release, std::memory_order_relaxed);
            }
            threadData[thread_id].nodeHp = NULL;
        }
        ASQTail.compare_exchange_weak(annOldTailAndCnt,
                                      {ann->batchReq.lastEnq, annOldTailAndCnt.cnt + ann->batchReq.enqsNum},
                                      std::memory_order_release, std::memory_order_relaxed);
        threadData[thread_id].nodeHp = NULL;
        UpdateHead(ann, thread_id);
    }

    /*
    * The UpdateHead method calculates successfulDeqsNum,
    * then determines the new head, and finally, updates ASQHead.
    */
    void UpdateHead(Ann<T> *ann, unsigned int thread_id) {
        int oldQueueSize = ann->oldTail.cnt - ann->oldHead.cnt;
        int successfulDeqsNum = ann->batchReq.deqsNum;
        Node<T> *newHeadNode;
        PtrCntOrAnn<T> anAnn;
        PtrCntOrAnn<T> temp;
        if (ann->batchReq.excessDeqsNum > oldQueueSize)
            successfulDeqsNum -= ann->batchReq.excessDeqsNum - oldQueueSize;
        if (successfulDeqsNum == 0) {
            anAnn.annAndTag = {1, ann};
            Ann<T> *a = anAnn.annAndTag.ann;
            unsigned int ta = anAnn.annAndTag.tag;
            ASQHead.compare_exchange_weak(anAnn, {ann->oldHead}, std::memory_order_release, std::memory_order_relaxed);
            return;
        }
        threadData[thread_id].nodeHp = ann->oldHead.node;
        //_memoryFence
        std::atomic_thread_fence(std::memory_order_release);
        if (threadData[thread_id].warning) {
            threadData[thread_id].warning = false;
            if (ASQHead.load(std::memory_order_relaxed).annAndTag.ann != ann) {
                threadData[thread_id].nodeHp = NULL;
                return;
            }
        }
        if (oldQueueSize > successfulDeqsNum)
            newHeadNode = GetNthNode(ann->oldHead.node, successfulDeqsNum, ann, thread_id);
        else
            newHeadNode = GetNthNode(ann->oldTail.node, successfulDeqsNum - oldQueueSize, ann, thread_id);
        if (newHeadNode == NULL) {
            threadData[thread_id].nodeHp = NULL;
            return;
        }
        T *lastDequeuedItem = newHeadNode->item;
        if (ASQHead.load(std::memory_order_relaxed).annAndTag.ann != ann) {
            threadData[thread_id].nodeHp = NULL;
            return;
        }
        ann->oldHead.node->item = lastDequeuedItem;
        threadData[thread_id].nodeHp = NULL;
        anAnn.annAndTag = {1, ann};
        temp = {newHeadNode, ann->oldHead.cnt + successfulDeqsNum};
        ASQHead.compare_exchange_weak(anAnn, temp, std::memory_order_release, std::memory_order_relaxed);
    }

    /*
    * This method gets a node and a number, and return the node in the n'th place to the node.
    */
    Node<T> *GetNthNode(Node<T> *node, unsigned int n, Ann<T> *ann, unsigned int thread_id) {
        for (int i = 0; i < n; i++) {
            node = node->next;
            //_compilerFence
            std::atomic_signal_fence(std::memory_order_release);
            if (threadData[thread_id].warning) {
                threadData[thread_id].warning = false;
                if (ASQHead.load(std::memory_order_relaxed).annAndTag.ann != ann)
                    return NULL;
            }
        }
        return node;
    }

    /*
    * Enqueue checks whether the thread-local operation queue opsQueue is empty.
    * If it is, it directly calls EnqueueToShared.
    * Otherwise, the pending operations in opsQueue must be applied before the current Enqueue is applied.
    * Hence, Enqueue calls FutureEnqueue with the required item, which in turn returns a future.
    * It then calls Evaluate with that future.
    * This results in applying all preceding pending operations, as well as applying the current operation.
    */
    void Enqueue(T *item, unsigned int thread_id) {
        if (threadData[thread_id].opsQueue.empty())
            EnqueueToShared(item, thread_id);
        else
            Evaluate(FutureEnqueue(item, thread_id), thread_id);
    }

    /*
    * Similar to Enqueue. If Dequeue succeeds, it returns the dequeued item,
    * otherwise (the queue is empty when the operation takes effect) it returns NULL.
    */
    T *Dequeue(unsigned int thread_id) {
        if (threadData[thread_id].opsQueue.empty())
            return DequeueFromShared(thread_id);
        else
            return Evaluate(FutureDequeue(thread_id), thread_id);
    }

    /*
    * FutureEnqueue calls AddToEnqsList, and also updates the local numbers of pending enqueue operations.
    * In addition, FutureEnqueue enqueues a FutureOp object representing an enqueue operation to the thread’s opsQueue.
    */
    Future<T> *FutureEnqueue(T *item, unsigned int thread_id) {
        AddToEnqsList(item, thread_id);
        ++(threadData[thread_id].enqsNum);
        return RecordOpAndGetFuture(ENQ, thread_id);
    }

    /*
    * Creates new node, with the item for the future enqueue,
    * adds the item to be enqueued to thread’s list of items pending to be enqueued
    */
    void AddToEnqsList(T *item, unsigned int thread_id) {
        Node<T> *node = MM.AllocateNode();
        node->item = item;
        node->next = NULL;
        if (threadData[thread_id].enqsHead == NULL)
            threadData[thread_id].enqsHead = node;
        else
            threadData[thread_id].enqsTail->next = node;
        threadData[thread_id].enqsTail = node;
    }

    /*
    * pushes the type of operation (enqueue or dequeue) and the future to the local queue of future operations.
    * returns this future type that was created.
    */
    Future<T> *RecordOpAndGetFuture(Type futureOpType, unsigned int thread_id) {
        Future<T> *future = new Future<T>();
        threadData[thread_id].opsQueue.push({future, futureOpType});
        return future;
    }

    /*
    * standard max function, which gets two numbers,
    * and returns the greater of them.
    */
    int max(int x, int y) {
        return (x > y) ? x : y;
    }

    /*
    * FutureDequeue updates the local numbers of pending dequeue operations and excess dequeues.
    * then enqueues a FutureOp object representing a dequeue operation to the thread’s opsQueue.
    */
    Future<T> *FutureDequeue(unsigned int thread_id) {
        ++(threadData[thread_id].deqsNum);
        threadData[thread_id].excessDeqsNum = max(threadData[thread_id].excessDeqsNum,
                                                  threadData[thread_id].deqsNum - threadData[thread_id].enqsNum);
        return RecordOpAndGetFuture(DEQ, thread_id);
    }

    void UndoFuture(unsigned int thread_id) {
        if (threadData[thread_id].opsQueue.empty())
            return;
        if (threadData[thread_id].opsQueue.back().type== ENQ) {
            --(threadData[thread_id].enqsNum);
            if (threadData[thread_id].enqsHead == threadData[thread_id].enqsTail) {
                MM.Free(threadData[thread_id].enqsTail);
                threadData[thread_id].enqsHead = threadData[thread_id].enqsTail = NULL;
            }
            else {
                Node<T> *node = threadData[thread_id].enqsHead;
                while (node->next != threadData[thread_id].enqsTail) {
                    node = node->next;
                }
                node->next = NULL;
                MM.Free(threadData[thread_id].enqsTail);
                threadData[thread_id].enqsTail = node;
            }
        }
        if (threadData[thread_id].opsQueue.back().type == DEQ) {
            --(threadData[thread_id].deqsNum);
        }
        threadData[thread_id].excessDeqsNum = PopQueueLastItem(&threadData[thread_id].opsQueue, thread_id);
    }

    /*
     * pop the last item of the queue (instead of regular pop),
     * returns the number of excess deqs.
     */
    unsigned int PopQueueLastItem (std::queue<FutureOp<T>>* queue, unsigned int thread_id) {
        std::queue<FutureOp<T>> tempQueue;
        while (queue->size()>1) {
            tempQueue.push(queue->front());
            queue->pop();
        }
        this->futureToDel[thread_id].push(queue->front().future);
        queue->pop();
        unsigned  int excessDeq = 0;
        unsigned  int deqs = 0;
        unsigned  int enqs = 0;
        while (!tempQueue.empty()) {
            queue->push(tempQueue.front());
            if (queue->front().type == ENQ) {
                enqs++;
            }
            if (queue->front().type == DEQ) {
                deqs++;
                excessDeq = max(excessDeq, deqs - enqs);
            }
			tempQueue.pop();
        }
        return excessDeq;
    }

    /*
     * Evaluate receives a future and ensures it is applied when the method returns.
     * a future may be evaluated by its creator thread only.
     * If the future has already been applied from the outset, its result is immediately returned.
     * Otherwise, it calls ExecuteAllPending.
     */
    T *Evaluate(Future<T> *future, unsigned int thread_id) {
        if (!future->isDone) {
            ExecuteAllPending(NULL, thread_id);
        }
        return future->result;
    }
	
	
	T *Evaluate(Future<T> *future, std::queue<T*>* deqList, unsigned int thread_id) {
        if (!future->isDone) {
            ExecuteAllPending(deqList, thread_id);
        }
        return future->result;
    }

    /*
     * ExecuteAllPending is called by Evaluate.
     * All locally-pending operations found in threadData.opsQueue are applied to the shared queue at once.
     * After the batch operation’s execution completes, while new operations may be applied to the shared queue by other threads,
     * the batch operation results are paired to the appropriate futures of operations in opsQueue.
     */
    void ExecuteAllPending(std::queue<T*>* deqList, unsigned int thread_id) {
        Node<T> *oldHeadNode;
        unsigned int successDeqsNum;
        T *lastDeqItem;
        if (threadData[thread_id].enqsNum == 0) {
            Three<unsigned int, Node<T> *, T *> three = ExecuteDeqsBatch(thread_id);
            successDeqsNum = three.item1;
            oldHeadNode = three.item2;
            lastDeqItem = three.item3;
            PairDeqFuturesWithResults(deqList, oldHeadNode, successDeqsNum, lastDeqItem, thread_id);
        } else {
            oldHeadNode = ExecuteBatch({threadData[thread_id].enqsHead, threadData[thread_id].enqsTail,
                                        threadData[thread_id].enqsNum,
                                        threadData[thread_id].deqsNum,
                                        threadData[thread_id].excessDeqsNum}, thread_id);
            PairFuturesWithResults(deqList, oldHeadNode, thread_id);
            threadData[thread_id].enqsHead = NULL;
            threadData[thread_id].enqsTail = NULL;
            threadData[thread_id].enqsNum = 0;
        }
        threadData[thread_id].deqsNum = 0;
        threadData[thread_id].excessDeqsNum = 0;
    }

    /*
    * PairFuturesWithResults receives the old head. It simulates the pending
    * operations one by one according to their original order, which is recorded in the thread’s opsQueue.
    */
    void PairFuturesWithResults(std::queue<T*>* deqList, Node<T> *oldHeadNode, unsigned int thread_id) {
        Node<T> *nextEnqNode = threadData[thread_id].enqsHead;
        Node<T> *currentHead = oldHeadNode;
        bool noMoreSuccessfulDeqs = false;
        bool shouldSetPrevDeqResult = false;
        Future<T> *lastSuccessfulDeqFuture = NULL;
        T *oldHeadItem = oldHeadNode->item;
        T *lastSuccessfulDeqItem;
        Node<T> *nodePrecedingDeqNode;
        while (!threadData[thread_id].opsQueue.empty()) {
            FutureOp<T> op = threadData[thread_id].opsQueue.front();
            threadData[thread_id].opsQueue.pop();
            if (op.type == ENQ)
                nextEnqNode = nextEnqNode->next;
            else {
                if (noMoreSuccessfulDeqs || currentHead->next == nextEnqNode)
                    op.future->result = NULL;
                else {
                    nodePrecedingDeqNode = currentHead;
                    currentHead = currentHead->next;
                    MM.Retire(nodePrecedingDeqNode, &nodesToDel[thread_id], threadData);
                    if (currentHead == threadData[thread_id].enqsTail)
                        noMoreSuccessfulDeqs = true;
                    if (shouldSetPrevDeqResult) {
                        lastSuccessfulDeqFuture->result = lastSuccessfulDeqItem;
						if (deqList != NULL) {
							deqList->push(lastSuccessfulDeqItem);
						}
                    } else {
                        shouldSetPrevDeqResult = true;
                    }
                    lastSuccessfulDeqFuture = op.future;
                    lastSuccessfulDeqItem = currentHead->item;
                }
            }
            op.future->isDone = true;
            this->futureToDel[thread_id].push(op.future);
        }
        if (shouldSetPrevDeqResult) {
            lastSuccessfulDeqFuture->result = oldHeadItem;
			if (deqList != NULL) {
				deqList->push(lastSuccessfulDeqItem);
				}
		}
		
    }

    /*
    * The ExecuteDeqsBatch method first assists a colliding ongoing batch operation if there is any.
    * It then calculates the new head and the number of successful dequeues.
    * If there is at least one successful dequeue,  pushes the shared queue’s head successfulDeqsNum nodes forward.
    * Then calls PairDeqFuturesWithResults.
    */
    Three<unsigned int, Node<T> *, T *> ExecuteDeqsBatch(unsigned int thread_id) {
        Node<T> *headNextNode;
        PtrCnt<T> oldHeadAndCnt;
        PtrCntOrAnn<T> oldHeadOrAnn;
        Node<T> *newHeadNode;
        unsigned int successfulDeqsNum;
        T *lastDeqItem = NULL;
        while (true) {
            oldHeadAndCnt = HelpAnnAndGetHead(thread_id);
            threadData[thread_id].nodeHp = oldHeadAndCnt.node;
            //_memoryFence
            std::atomic_thread_fence(std::memory_order_release);
            if (threadData[thread_id].warning) {
                threadData[thread_id].warning = false;
                goto cleanup;
            }
            newHeadNode = oldHeadAndCnt.node;
            successfulDeqsNum = 0;
            for (int i = 0; i < threadData[thread_id].deqsNum; i++) {
                headNextNode = newHeadNode->next;
                //_compilerFence
                std::atomic_signal_fence(std::memory_order_release);
                if (threadData[thread_id].warning) {
                    threadData[thread_id].warning = false;
                    goto cleanup;
                }
                if (headNextNode == NULL)
                    break;
                ++successfulDeqsNum;
                newHeadNode = headNextNode;
            }
            if (successfulDeqsNum == 0) {
                lastDeqItem = NULL;
                break;
            }
            lastDeqItem = newHeadNode->item;
            oldHeadOrAnn.ptrCnt = oldHeadAndCnt;
            if (ASQHead.compare_exchange_weak(oldHeadOrAnn, {{newHeadNode, oldHeadAndCnt.cnt + successfulDeqsNum}},
                                              std::memory_order_release, std::memory_order_relaxed))
                break;
            cleanup:
            threadData[thread_id].nodeHp = NULL;
        }
        threadData[thread_id].nodeHp = NULL;
        return Three < unsigned int, Node<T> *, T * > (successfulDeqsNum, oldHeadAndCnt.node, lastDeqItem);
    }


    /*
    * This function pairs the successfully-dequeued-items
    * to futures of the appropriate operations in opsQueue. The remaining future dequeues are unsuccessful,
    * thus their results are set to NULL.
    */
    void PairDeqFuturesWithResults(std::queue<T*>* deqList, Node<T> *oldHeadNode, unsigned int successfulDeqsNum, T *lastDeqItem,
                                   unsigned int thread_id) {
        Node<T> *currentHead;
        Node<T> *nodePrecedingDeqNode;
        FutureOp<T> op;
        if (successfulDeqsNum > 0) {
            currentHead = oldHeadNode;
            for (int i = 0; i < successfulDeqsNum - 1; i++) {
                nodePrecedingDeqNode = currentHead;
                currentHead = currentHead->next;
                MM.Retire(nodePrecedingDeqNode, &nodesToDel[thread_id], threadData);
                op = threadData[thread_id].opsQueue.front();
                threadData[thread_id].opsQueue.pop();
                op.future->result = currentHead->item;
                op.future->isDone = true;
				if (deqList != NULL) {
					deqList->push(currentHead->item);
				}
                this->futureToDel[thread_id].push(op.future);
            }
            MM.Retire(currentHead, &nodesToDel[thread_id], threadData);
            op = threadData[thread_id].opsQueue.front();
            threadData[thread_id].opsQueue.pop();
            op.future->result = lastDeqItem;
            op.future->isDone = true;
			if (deqList != NULL) {
				deqList->push(lastDeqItem);
			}			
            this->futureToDel[thread_id].push(op.future);
        }
        for (int i = 0; i < threadData[thread_id].deqsNum - successfulDeqsNum; i++) {
            op = threadData[thread_id].opsQueue.front();
            threadData[thread_id].opsQueue.pop();
            op.future->result = NULL;
            op.future->isDone = true;
            this->futureToDel[thread_id].push(op.future);
        }
    }


    /*
    Deletes all memory that saved in the queue, including nodes, anns, and futures.
    */
    ~Queue() {
        Node<T> *node = ASQHead.load(std::memory_order_relaxed).ptrCnt.node;
        Node<T> *nodeToDel;
        while (node != NULL) {
            nodeToDel = node;
            node = node->next;
            MM.Retire(nodeToDel, &nodesToDel[0], 0);
        }
        for (int i = 0; i < numOfThreads; i++) {
            while (!this->futureToDel[i].empty()) {
                delete (futureToDel[i].front());
                futureToDel[i].pop();
            }
        }
        for (int i = 0; i < numOfThreads; i++) {
            while (!this->nodesToDel[i].empty()) {
                MM.Free(nodesToDel[i].front());
                nodesToDel[i].pop();
            }
        }
        for (int i = 0; i < numOfThreads; i++) {
            while (!this->annsToDel[i].empty()) {
                MM.Free(annsToDel[i].front());
                annsToDel[i].pop();
            }
        }
        delete[](futureToDel);
        delete[](nodesToDel);
        delete[](annsToDel);
        delete[](threadData);
    }
};


#endif //BQ_QUEUE_H
