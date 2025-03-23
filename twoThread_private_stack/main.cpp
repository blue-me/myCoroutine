#include<iostream>
#include<vector>
#include<chrono>
#include"Scheduler.h"
using namespace std;
#define QUEUE_LENGTH 1024
vector<int> mq(QUEUE_LENGTH,0);
int read = 0;
int write = 0;
int number = 1;
bool flag = true;//true means empty,false means full

/**
 * @brief 协程函数，每次调用就返回一个斐波那契数字，最多输出 n 个
 */
void coFib(Coroutine& co, void* nptr) {
    int n = *(int*)nptr;
    int a = 1, b = 1;
    int c;
    if (n < 1) return;
    cout << a << endl;
    co.coYield();

    if (n < 2) return;
    cout << b << endl;
    co.coYield();

    for (int i = 2; i <= n; ++i) {
        c = a + b;
        cout << c << endl;
        co.coYield();
        a = b;
        b = c;
    }
}

/**
 * @brief 协程函数，打印数字
*/
void coEcho(Coroutine& co, void* nptr1, void *nptr2) {
    int n1 = *(int*)nptr1;
    int n2 = *(int*)nptr2;

    for(int i=0; i<n1; i++) {
        std::cout << n2++ << std::endl;
        co.coYield();
    }
}

/*
   生产者模型
*/
void producer(Coroutine& co)
{
    while(number < 20000000)
    {
        //std::cout<<number<<std::endl;
        if(read != write || flag == true)
        {
            mq[write] = number;
            write = (write + 1) % 10;
            ++number;
            if(write == read || number == 20000000) flag = false;
        }
        else std::cout<<"Mq is full"<<std::endl;
        co.coYield();
    }
}
/*
    消费者模型
*/
void consumer(Coroutine& co)
{
    while(number < 20000000 || flag == false)
    {
        while(read != write || flag == false)
        {
            //std::cout<<"Consumer: "<<mq[read]<<std::endl;
            read = (read + 1) % 10;
            if(read == write) flag = true;
        }
        //std::cout<<"Mq is empty now."<<std::endl;
        co.coYield();
    }
}

int main() {
    Scheduler scheduler;
    // int n1 = 10, n2 = 20;
    // void* ptr1 = (void*)(&n1), *ptr2 = (void*)(&n2);
    // scheduler.addCoroutine(make_shared<Coroutine>(coFib, ptr1));
    // scheduler.addCoroutine(make_shared<Coroutine>(coEcho, ptr1, ptr2));
    for(int i = 0;i<5;++i)
    {
        scheduler.addCoroutine(make_shared<Coroutine>(producer));
    }
    scheduler.addCoroutine(make_shared<Coroutine>(consumer));
    auto a = std::chrono::high_resolution_clock::now();
    scheduler.run();
    auto b = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(b - a);
    long long int diff = duration.count();
    std::cout << "Time difference: " << diff << " ms" << std::endl;
    return 0;
}