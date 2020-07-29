#include <string>
#include <thread>
#include "headers/Queue.h"

#define n 10
#define m 10
#define receiverNum 4
#define handlerNum 4

class Package {
public:
    unsigned int trackingNumber;
    unsigned int weight;
    std::string adresseeAdress;
    std::string adresseeName;
    unsigned int phoneNumber;
    std::string senderAdress;
    std::string senderName;
};

void receivingPackages(Queue<Package> *handlingQueue, unsigned int worker_id) {
    while (true) {
        int i = 0;
        while (!timeout && i < n) {
            Package *p = GetPackage();
            handlingQueue->FutureEnqueue(p, worker_id);
            i++;
        }
        if (Stop()) break;
    }
}

void handlingPackages(Queue<Package> *handlingQueue, unsigned int worker_id) {
    while (true) {
        int i = 0;
        Future<Package> *future;
        std::queue<Package *> *Packages = new std::queue<Package *>();
        while (!timeout && i < m) {
            future = handlingQueue->FutureDequeue(worker_id);
            i++;
        }
        handlingQueue->Evaluate(future, Packages, worker_id);
        hadlePackages(Packages);
        if (Stop()) break;
    }
}


int main() {
    Queue<Package> *PackageHandleQueue = new queue<Package>(handlerNum + receiverNum);
    std::thread receivers[receiverNum];
    std::thread handlers[handlerNum];
    for (int i = 0; i < receiverNum; i++) {
        receivers[i] = std::thread(receivingPackages, PackageHandleQueue, i);
    }
    for (int i = 0; i < handlerNum; i++) {
        handlers[i] = std::thread(handlingPackages, PackageHandleQueue, i + receiverNum);
    }
    for (int i = 0; i < receiverNum; ++i) {
        receivers[i].join();
    }
    for (int i = 0; i < handlerNum; ++i) {
        handlers[i].join();
    }
    delete (PackageHandleQueue);
    return 0;
}
