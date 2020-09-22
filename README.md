# BatchQueue

## Documents
* [User Manual](docs/User%20manual.docx) - includes also programmer guide and documentation.
* [Batch Queue pptx](docs/Batch%20Queue.pptx) - project presentation.
* [Performance Evaluation](docs/BQueue.xlsx) - results of 240 executions, by threads number and batch size.
* [Performance Results - Graphs](docs/Performance%and%Evaluation.docx) - graphs of performance, based on results of 240 executions. 

#### How to compile?  
`g++ -std=c++11  -pthread -march=native  *.cpp -o prog_name`

### About the project
This project implements the queue from the article "BQ: Lock-Free Queue with Batching"
by Gal Milman et al. The article also provides Linearizability Proof.
This queue functions as a regular lock-free queue,
allowing thread-safe enqueue and dequeue methods.
In addition, this queue allows using future operations – enqueues and dequeues
that can be delayed, for later execution.

This project was implemented using the C++11 programming language.

The project adds two additional methods to the implementation in the article.
One is undoing a future operation, which allow to remove the last future enqueue
or future dequeue that was added for later execution and was not executed yet.
The second is getting a queue of all the items that was dequeued when executing a batch.
I have tried to avoid unnecessary overhead when not using these methods,
compared to the original implementation.


#### Notes:
1.  Due to some bugs in old versions of gcc when using atomic library  
    this project will not run perfectly on every platform.  
    For example, using "compare_exchange_weak(obj, expected, desired)" (or strong)
    might fail even if obj=expected and there are no threads to interrupt.  
    I was trying to fix it in order to make it work for any platform.
    Yet, there is a chance to have some problems using this queue with older GCC version.
