#pragma once
#include<ucontext.h>
#include<stddef.h>
#include<functional>
#include<cassert>
#include<string.h>
#include<iostream>

class Coroutine;
typedef std::function<void(Coroutine&)> CoroutineFunc;

/* 每个协程栈的大小，10k */
#define CO_STACK_SIZE (1024*1024)

/**
 * @brief 实现一个简易的私有栈协程
*/
class Coroutine {
public:
    enum CO_STATE {CO_TERM = 0, CO_READY, CO_RUNNING, CO_SUSPEND};

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

private: 
    static void coFuncExecu(uint32_t lowBits, uint32_t highBits);
    void        InitCoroutine();
    void        freeCoroutine();

private:
    /* 协程上下文 */
    ucontext_t*   uc_;
    /* 调用者协程的上下文 */
    ucontext_t*   ucCaller_;

    /* 协程栈相关信息 */
    char*         stack_;
    ucontext_t*   uc_link_;

    /* 协程其它信息 */
    CO_STATE      coState_;
    CoroutineFunc func_;
};
