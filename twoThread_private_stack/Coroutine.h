#pragma once
#include<ucontext.h>
#include<stddef.h>
#include<functional>
#include<cassert>
#include<string.h>
#include<iostream>

class Coroutine;
typedef std::function<void(Coroutine&)> CoroutineFunc;
enum CO_STATE {CO_TERM = 0, CO_READY, CO_RUNNING, CO_SUSPEND, CO_OVERLOAD, CO_LIGHTLOAD,CO_OVERLOAD_WORKING,CO_LIGHTLOAD_WORKING};

/* 每个协程栈的初始大小，10k(以sizeof(char)为单位) */
#define CO_STACK_SIZE (1024 * 10)
#define USAGE_UPLIMITED 0.76
#define USAGE_DOWNLIMITED 0.24
#define UP_LIMITED 2
#define UP_DOWN 5
#define INCREASE_RATIO 2
#define SHRINK_RATIO 0.5

/**
 * @brief 实现一个简易的私有栈协程
*/
class Coroutine {
public:
    // enum CO_STATE {CO_TERM = 0, CO_READY, CO_RUNNING, CO_SUSPEND, CO_OVERLOAD, CO_LIGHTLOAD};

public:
    template<class F, class... Args>
    Coroutine(F&& func, Args&&... args):
        uc_(nullptr),
        stack_(nullptr),
        uc_link_(nullptr),
        coState_(CO_READY) { func_ =  std::bind(std::forward<F>(func), std::placeholders::_1, std::forward<Args>(args)...); }

    ~Coroutine();

    void coYield();
    void coResume();
    bool isCoOnTerm() { return coState_ == CO_TERM; }
    void checkSpace();

    int getSpace();
    CO_STATE getCoState();
    int getCapacity();

    void increaseSpace();
    void shrinkSpace();
    void changeCoState(CO_STATE state);


private: 
    static void coFuncExecu(uint32_t lowBits, uint32_t highBits);
    void        InitCoroutine(int id);
    void        freeCoroutine();
    

private:
    /* 协程上下文 */
    ucontext_t*   uc_;
    /* 调用者协程的上下文 */
    ucontext_t*   ucCaller_;

    /* 协程栈相关信息 */
    void*         stack_;
    ucontext_t*   uc_link_;
    int capacity_;
    int usedSpace_ = 0;
    int round = 0;


    /* 协程其它信息 */
    CO_STATE      coState_;
    CoroutineFunc func_;
    int id;
};
