//
// Created by BAR on 22/04/2020.
//

#ifndef BQ_PTRCNT_H
#define BQ_PTRCNT_H

#include "BatchRequest.h"


/*
* PtrCnt contains a Pointer to a node, and a counter.
* A struct for the head/tail of the queue. 
*/
template<typename T>
class PtrCnt {
public:
    Node<T> *node;
    unsigned int cnt;
};


/*
* An Ann object represents an announcement.
* It contains a BatchRequest instance, with all the details required to
* execute the batch operation it stands for. Thus, any operation that
* encounters an announcement may help the related batch operation
* complete before proceeding with its own operation.
* In addition to information regarding the batch of operations to execute,
* Ann includes oldHead, the value of the head pointer (and dequeue counter)
* before the announcement was installed, and oldTail,
* an entry for the tail pointer (and enqueue counter)
* of the queue right before the batch is applied.
*/
template<typename T>
class Ann {
public:
    BatchRequest<T> batchReq;
    PtrCnt<T> oldHead;
    PtrCnt<T> oldTail;
};

/*
* This struct contains an tag, for noting this is an Ann,
* when using union (if tag == 1, this is an Ann, since
* the corresponding attribute is a pointer for a valid adress,
* which is necessarily even, and cannot have 1 as LSB).
*/
template<typename T>
class AnnAndTag {
public:
    unsigned int tag;
    Ann<T> *ann;
};


/*
* 16-byte union that may consist of either PtrCnt or an 8-byte tag and an 8-byte Ann pointer. 
* This used for the head of the queue, which can have Ann, noting a future batch.
*/
template<typename T>
union PtrCntOrAnn {
public:
    PtrCnt<T> ptrCnt;
    AnnAndTag<T> annAndTag;
};


#endif //BQ_PTRCNT_H
