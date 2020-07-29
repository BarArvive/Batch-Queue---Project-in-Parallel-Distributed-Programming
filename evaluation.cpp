#include <iostream>
#include "headers/Queue.h"
#include <thread>
#include <cstdlib>

using namespace std;


const int numOfThreads = 8;
const int batchSize = 64;
bool flag[numOfThreads] = {false};
int opNum[numOfThreads] = {0};
int e1 = 1;
bool isDone = false;
bool isEnq[14] = {true, true, false, false, false, true, true, true, true, false, true, false, false, false};


void enqueue(Queue<int> *bq, unsigned int thread_id) {
    flag[thread_id] = true;
    bool isTrue = true;
    while (true) {
        for (bool i : flag) {
            isTrue = isTrue && i;
        }
        if (isTrue) break;
        isTrue = true;
    }
    Future<int> *f;
    while (!isDone) {
        for (int i = 0; i < batchSize; i++) {
            if (isEnq[i % 14]) {
                f = bq->FutureEnqueue(&e1, thread_id);
            } else {
                f = bq->FutureDequeue(thread_id);
            }
        }
        opNum[thread_id] += 1;
        bq->Evaluate(f, thread_id);
    }
}


int main() {
    auto *bq = new Queue<int>(numOfThreads);
	std::thread t[numOfThreads];
	for (int i = 0; i < numOfThreads; i++) {
		t[i] = std::thread(enqueue, bq, i);
	}
	sleep(3);
	isDone = true;	
	for (int i = 0; i < numOfThreads; i++) {
		t[i].join();
	}	
    int sum = 0;
    for (int i : opNum) {
        sum += i * batchSize;
    }
    std::cout << "Number of op, Thread Sum:" << sum / 3 << std::endl;
    delete (bq);
    return 0;
}