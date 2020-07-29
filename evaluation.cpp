#include <iostream>
#include "Queue.h"
#include <thread>
#include <stdlib.h>

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
		for (int i = 0; i < numOfThreads; i++) {
			isTrue = isTrue && flag[i];
		}
		if (isTrue) break;
		isTrue = true;
	}
	Future<int>* f;
    while (!isDone) {
		for (int i = 0; i < batchSize; i++) {
			if (isEnq[i%14]) {
				f = bq->FutureEnqueue(&e1, thread_id);
			}
			else {
				f = bq->FutureDequeue(thread_id);
			}
		}
		opNum[thread_id] += 1;
		bq->Evaluate(f, thread_id);
    }
}



int main() {
    Queue<int> *bq = new Queue<int>(numOfThreads);
    thread t1(enqueue, bq, 0);
    thread t2(enqueue, bq, 1);
    thread t3(enqueue, bq, 2);
    thread t4(enqueue, bq, 3);
	thread t5(enqueue, bq, 4);
    thread t6(enqueue, bq, 5);
    thread t7(enqueue, bq, 6);
    thread t8(enqueue, bq, 7);
/*    thread t9(enqueue, bq, 8);
    thread t10(enqueue, bq, 9);
    thread t11(enqueue, bq, 10);
    thread t12(enqueue, bq, 11);
    thread t13(enqueue, bq, 12);
    thread t14(enqueue, bq, 13);
    thread t15(enqueue, bq, 14);
    thread t16(enqueue, bq, 15);
    thread t17(enqueue, bq, 16);
    thread t18(enqueue, bq, 17);
    thread t19(enqueue, bq, 18);
    thread t20(enqueue, bq, 19);
	thread t21(enqueue, bq, 20);
    thread t22(enqueue, bq, 21);
    thread t23(enqueue, bq, 22);
    thread t24(enqueue, bq, 23);
    thread t25(enqueue, bq, 24);
    thread t26(enqueue, bq, 25);
    thread t27(enqueue, bq, 26);
    thread t28(enqueue, bq, 27);
    thread t29(enqueue, bq, 28);
    thread t30(enqueue, bq, 29);
    thread t31(enqueue, bq, 30);
    thread t32(enqueue, bq, 31);
    thread t33(enqueue, bq, 32);
    thread t34(enqueue, bq, 33);
    thread t35(enqueue, bq, 34);
    thread t36(enqueue, bq, 35);
	thread t37(enqueue, bq, 36);
    thread t38(enqueue, bq, 37);
    thread t39(enqueue, bq, 38);
    thread t40(enqueue, bq, 39);
    thread t41(enqueue, bq, 40);
    thread t42(enqueue, bq, 41);
    thread t43(enqueue, bq, 42);
    thread t44(enqueue, bq, 43);
    thread t45(enqueue, bq, 44);
    thread t46(enqueue, bq, 45);
    thread t47(enqueue, bq, 46);
    thread t48(enqueue, bq, 47);
    thread t49(enqueue, bq, 48);
    thread t50(enqueue, bq, 49);
    thread t51(enqueue, bq, 50);
    thread t52(enqueue, bq, 51);
	thread t53(enqueue, bq, 52);
    thread t54(enqueue, bq, 53);
    thread t55(enqueue, bq, 54);
    thread t56(enqueue, bq, 55);
    thread t57(enqueue, bq, 56);
    thread t58(enqueue, bq, 57);
    thread t59(enqueue, bq, 58);
    thread t60(enqueue, bq, 59);
    thread t61(enqueue, bq, 60);
    thread t62(enqueue, bq, 61);
    thread t63(enqueue, bq, 62);
    thread t64(enqueue, bq, 63);
   thread t65(enqueue, bq, 64);
    thread t66(enqueue, bq, 65);
    thread t67(enqueue, bq, 66);
    thread t68(enqueue, bq, 67);
	thread t69(enqueue, bq, 68);
    thread t70(enqueue, bq, 69);
    thread t71(enqueue, bq, 70);
    thread t72(enqueue, bq, 71);
    thread t73(enqueue, bq, 72);
    thread t74(enqueue, bq, 73);
    thread t75(enqueue, bq, 74);
    thread t76(enqueue, bq, 75);
    thread t77(enqueue, bq, 76);
    thread t78(enqueue, bq, 77);
    thread t79(enqueue, bq, 78);
    thread t80(enqueue, bq, 79);
    thread t81(enqueue, bq, 80);
    thread t82(enqueue, bq, 81);
    thread t83(enqueue, bq, 82);
    thread t84(enqueue, bq, 83);
	thread t85(enqueue, bq, 84);
    thread t86(enqueue, bq, 85);
    thread t87(enqueue, bq, 86);
    thread t88(enqueue, bq, 87);
    thread t89(enqueue, bq, 88);
    thread t90(enqueue, bq, 89);
    thread t91(enqueue, bq, 90);
    thread t92(enqueue, bq, 91);
    thread t93(enqueue, bq, 92);
    thread t94(enqueue, bq, 93);
    thread t95(enqueue, bq, 94);
    thread t96(enqueue, bq, 95);
    thread t97(enqueue, bq, 96);
    thread t98(enqueue, bq, 97);
    thread t99(enqueue, bq, 98);
    thread t100(enqueue, bq, 99);
	thread t101(enqueue, bq, 100);
    thread t102(enqueue, bq, 101);
    thread t103(enqueue, bq, 102);
    thread t104(enqueue, bq, 103);
    thread t105(enqueue, bq, 104);
    thread t106(enqueue, bq, 105);
    thread t107(enqueue, bq, 106);
    thread t108(enqueue, bq, 107);
    thread t109(enqueue, bq, 108);
    thread t110(enqueue, bq, 109);
    thread t111(enqueue, bq, 110);
    thread t112(enqueue, bq, 111);
    thread t113(enqueue, bq, 112);
    thread t114(enqueue, bq, 113);
    thread t115(enqueue, bq, 114);
    thread t116(enqueue, bq, 115);
	thread t117(enqueue, bq, 116);
    thread t118(enqueue, bq, 117);
    thread t119(enqueue, bq, 118);
    thread t120(enqueue, bq, 119);
    thread t121(enqueue, bq, 120);
    thread t122(enqueue, bq, 121);
    thread t123(enqueue, bq, 122);
    thread t124(enqueue, bq, 123);
    thread t125(enqueue, bq, 124);
    thread t126(enqueue, bq, 125);
    thread t127(enqueue, bq, 126);
    thread t128(enqueue, bq, 127);*/
	sleep(3);
	isDone = true;	
	t1.join();
    t2.join();
   t3.join();
    t4.join();
	t5.join();
    t6.join();
    t7.join();
    t8.join();
/*	t9.join();
    t10.join();
    t11.join();
    t12.join();
	t13.join();
    t14.join();
    t15.join();
    t16.join();
	t17.join();
    t18.join();
    t19.join();
    t20.join();
	t21.join();
    t22.join();
    t23.join();
    t24.join();
	t25.join();
    t26.join();
    t27.join();
    t28.join();
	t29.join();
    t30.join();
    t31.join();
    t32.join();
	t33.join();
    t34.join();
    t35.join();
    t36.join();
	t37.join();
    t38.join();
    t39.join();
    t40.join();
	t41.join();
    t42.join();
    t43.join();
    t44.join();
	t45.join();
    t46.join();
    t47.join();
    t48.join();
	t49.join();
    t50.join();
    t51.join();
    t52.join();
	t53.join();
    t54.join();
    t55.join();
    t56.join();
	t57.join();
    t58.join();
    t59.join();
    t60.join();
	t61.join();
    t62.join();
    t63.join();
    t64.join();
	t65.join();
    t66.join();
    t67.join();
    t68.join();
	t69.join();
    t70.join();
    t71.join();
    t72.join();
	t73.join();
    t74.join();
    t75.join();
    t76.join();
	t77.join();
    t78.join();
    t79.join();
    t80.join();
	t81.join();
    t82.join();
    t83.join();
    t84.join();
	t85.join();
    t86.join();
    t87.join();
    t88.join();
	t89.join();
    t90.join();
    t91.join();
    t92.join();
	t93.join();
    t94.join();
    t95.join();
    t96.join();
	t97.join();
    t98.join();
    t99.join();
    t100.join();
	t101.join();
    t102.join();
    t103.join();
    t104.join();
	t105.join();
    t106.join();
    t107.join();
    t108.join();
	t109.join();
    t110.join();
    t111.join();
    t112.join();
	t113.join();
    t114.join();
    t115.join();
    t116.join();
	t117.join();
    t118.join();
    t119.join();
    t120.join();
	t121.join();
    t122.join();
    t123.join();
    t124.join();
	t125.join();
    t126.join();
    t127.join();
    t128.join();*/
	int sum = 0;
	for (int i = 0; i < numOfThreads; i++) {
		sum += opNum[i]*batchSize;
	}
	std::cout << "Number of op, Thread Sum:" << sum/3 << std::endl;
    delete(bq);
    return 0;
}