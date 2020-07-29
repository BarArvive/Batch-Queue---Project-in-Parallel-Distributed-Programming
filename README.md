# BatchQueue

How to compile?
g++ -std=c++11  -pthread -march=native  *.cpp -o prog_name

Notes:
1. Due to some bugs in old versions of gcc when using atomic library, this project will not run perfectly on every platform.
 For example, using "compare_exchange_weak(obj, expected, desired)" (or strong)
 might fail even if obj=expected, and there are no threads to interrupt.
 I was trying to fix it, in order to make it work for any platform.
 Yet, there is a chance to have some problems using this queue with older GCC version.
2. 
