//
// Created by BAR on 14/06/2020.
//

#ifndef BQ_TESTING_H
#define BQ_TESTING_H

#include <string>
#include <iostream>
#include "Queue.h"
#include <queue>
#include <cstdlib>

enum Result_e {
    SUCCESS, ENQUEUE_FAIL, DEQUEUE_FAIL, FUTURE_ENQUEUE_FAIL, FUTURE_DEQUEUE_FAIL, EVALUATE_LIST_FAIL
};

class Test {
public:
    Queue<int> *queue1;
    Queue<int> *queue2;
    Queue<int> *queue3;
    Queue<int> *queue4;
    Queue<int> *queue5;
    Queue<int> *queue6;
    Queue<int> *queue7;

    Test() {
        queue1 = new Queue<int>(1);
        queue2 = new Queue<int>(1);
        queue3 = new Queue<int>(1);
        queue4 = new Queue<int>(1);
        queue5 = new Queue<int>(1);
        queue6 = new Queue<int>(1);
        queue7 = new Queue<int>(1);
    }

    Result_e EnqTest() const {
        int a = 1;
        queue1->Enqueue(&a, 0);
        int *t = queue1->ASQTail.load(std::memory_order_relaxed).node->item;
        if (*t != 1) {
            std::cout << "FAIL AT 27::EnqTest" << std::endl;
            return ENQUEUE_FAIL;
        }
        int b = 2;
        queue1->Enqueue(&a, 0);
        queue1->Enqueue(&b, 0);
        t = queue1->ASQTail.load(std::memory_order_relaxed).node->item;
        if (*t != 2) {
            std::cout << "FAIL AT 35::EnqTest" << std::endl;
            return ENQUEUE_FAIL;
        }
        int num[50];
        for (int i = 0; i < 50; i++) {
            num[i] = i;
            queue1->Enqueue(num + i, 0);
        }
        t = queue1->ASQTail.load(std::memory_order_relaxed).node->item;
        if (*t != 49) {
            std::cout << "FAIL AT 45::EnqTest" << std::endl;
            return ENQUEUE_FAIL;
        }
        int count = 0;
        int null_nodes_counter = 0;
        Node<int> *node = queue1->ASQHead.load(std::memory_order_relaxed).ptrCnt.node;
        node = node->next;
        while (node != nullptr) {
            if (node->item == nullptr) {
                null_nodes_counter++;
                node = node->next;
                continue;
            }
            count++;
            node = node->next;
        }
        if (count != 53) {
            std::cout << "FAIL AT 61::EnqTest, count: " << count << std::endl;
            return ENQUEUE_FAIL;
        }
        if (null_nodes_counter != 0) {
            std::cout << "FAIL AT 65::EnqTest" << std::endl;
            return ENQUEUE_FAIL;
        }
        while (queue1->Dequeue(0) != nullptr) {}
        return SUCCESS;
    }

    Result_e DeqTest() const {
        int num[100];
        for (int i = 0; i < 100; i++) {
            num[i] = i;
            queue2->Enqueue(num + i, 0);
        }
        for (int i = 0; i < 100; i++) {
            int *p = queue2->Dequeue(0);
            if (*p != i) {
                std::cout << "FAIL AT 81::DeqTest, it: " << i << std::endl;
                return DEQUEUE_FAIL;
            }
        }

        return SUCCESS;
    }

    Result_e FutureEnqTest() const {
        //std::cout << "A start queue:" << std::endl;
        //printQueue(queue3);
        int num[50];
        Future<int> *f[51];
        for (int i = 0; i < 50; i++) {
            num[i] = i;
            f[i] = queue3->FutureEnqueue(num + i, 0);
        }
        //f[51] =  queue3->FutureDequeue(0);
        for (int i = 0; i < 50; i++) {
            if (f[i]->isDone) {
                std::cout << "FAIL AT 99::FutureEnqTest, it: " << i << std::endl;
                return FUTURE_ENQUEUE_FAIL;
            }
        }
        queue3->Evaluate(f[1], 0);
        for (int i = 0; i < 50; i++) {
            if (!f[i]->isDone) {
                std::cout << "FAIL AT 106::FutureEnqTest, it: " << i << std::endl;
                return FUTURE_ENQUEUE_FAIL;
            }
        }
        //std::cout << "A Full queue:" << std::endl;
        //printQueue(queue3);
        int count = 0;
        int null_nodes_counter = 0;
        Node<int> *node = queue3->HelpAnnAndGetHead(0).node;
        //node = NULL;
        node = node->next;
        while (node != nullptr) {
            if (node->item == nullptr) {
                null_nodes_counter++;
                node = node->next;
                continue;
            }
            count++;
            node = node->next;
        }
        if (count != 50) {
            std::cout << "FAIL AT 123::FutureEnqTest, count is: " << count << std::endl;
            return FUTURE_ENQUEUE_FAIL;
        }
        if (null_nodes_counter != 0) {
            std::cout << "FAIL AT 127::FutureEnqTest" << std::endl;
            return FUTURE_ENQUEUE_FAIL;
        }
        while (queue3->Dequeue(0) != nullptr) {}
        return SUCCESS;
    }

    Result_e FutureDeqTest() const {
        //      std::cout << "A start queue:" << std::endl;
        //      printQueue(queue4);
        int num[50];
        Future<int> *f[50];
        for (int i = 0; i < 50; i++) {
            num[i] = i;
            queue4->Enqueue(num + i, 0);
        }
        //     std::cout << "A Full queue:" << std::endl;
        //     printQueue(queue4);
        for (auto &i : f) {
            i = queue4->FutureDequeue(0);
        }
        for (int i = 0; i < 50; i++) {
            if (f[i]->isDone) {
                std::cout << "FAIL AT 146::FutureDeqTest, it: " << i << std::endl;
                return FUTURE_DEQUEUE_FAIL;
            }
        }
        queue4->Evaluate(f[0], 0);
        for (int i = 0; i < 50; i++) {
            if (!f[i]->isDone) {
                std::cout << "FAIL AT 153::FutureDeqTest, it: " << i << std::endl;
                return FUTURE_DEQUEUE_FAIL;
            }
            if (*f[i]->result != i) {
                std::cout << "FAIL AT 157::FutureDeqTest, it: " << i << "result: " << *f[i]->result << std::endl;
                return FUTURE_DEQUEUE_FAIL;
            }
        }
        return SUCCESS;
    }


    Result_e FutureUndoTest() const {
        int num[3];
        Future<int> *f[6];
        for (int i = 0; i < 3; i++) {
            num[i] = i;
            f[i] = queue6->FutureEnqueue(num + i, 0);
        }
        for (int i = 0; i < 3; i++) {
            f[i + 3] = queue6->FutureDequeue(0);
        }
        queue6->UndoFuture(0);
        queue6->UndoFuture(0);
        queue6->UndoFuture(0);
        queue6->UndoFuture(0);
        queue6->UndoFuture(0);
        //queue6->UndoFuture(0);
        queue6->Evaluate(f[0], 0);
        //	printQueue(queue6);
        return SUCCESS;
    }


    Result_e EvaluateListTest() const {
        int num[6];
        Future<int> *f[12];
        for (int i = 0; i < 6; i++) {
            num[i] = i;
            f[i] = queue7->FutureEnqueue(num + i, 0);
        }
        queue7->Evaluate(f[0], 0);
        for (int i = 0; i < 6; i++) {
            f[i + 6] = queue7->FutureDequeue(0);
        }
        auto *q1 = new std::queue<int *>();
        queue7->Evaluate(f[6], q1, 0);
        int i = 0;
        if (q1->size() != 6) {
            std::cout << "Evaluate list fail: size is " << q1->size() << ", not 6" << std::endl;
            return EVALUATE_LIST_FAIL;
        }
        while (!q1->empty()) {
            if (*q1->front() != i) {
                std::cout << "Evaluate list fail: " << *q1->front() << " != " << i << std::endl;
                return EVALUATE_LIST_FAIL;
            }
            q1->pop();
            i++;
        }
        delete (q1);
        return SUCCESS;
    }


    static void printQueue(Queue<int> *queue5) {
        Node<int> *node = queue5->ASQHead.load(std::memory_order_relaxed).ptrCnt.node;
        node = node->next; //head is dummy!
        while (node != nullptr) {
            if (node->item == nullptr) {
                node = node->next;
                continue;
            }
            std::cout << *node->item << "->";
            node = node->next;
        }
        std::cout << "||" << std::endl;
    }

    ~Test() {
        delete (queue1);
        delete (queue2);
        delete (queue3);
        delete (queue4);
        delete (queue5);
        delete (queue6);
        delete (queue7);
    }
};

#endif //BQ_TESTING_H
