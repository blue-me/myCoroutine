#include "Coroutine.h"

/**
 * @brief 析构
*/
Coroutine::~Coroutine() {
    freeCoroutine();
}

/**
 * @brief 释放协程资源
*/
void Coroutine::freeCoroutine() {
    if(uc_) free(uc_);
    if(ucCaller_) free(ucCaller_);
    if(stack_) free(stack_);
}

/**
 * @brief 封装协程任务
*/
void Coroutine::coFuncExecu(uint32_t lowBits, uint32_t highBits) {
    void* ptr = (void*)(((uintptr_t)highBits << 32) | (uintptr_t)lowBits);
    Coroutine* coroutine = (Coroutine*)ptr;
    coroutine->func_(*coroutine);
    coroutine->coState_ = CO_TERM;
    swapcontext(coroutine->uc_, coroutine->ucCaller_);
}

/**
 * @brief 初始化 ucp
*/
void Coroutine::InitCoroutine() {
    uc_     = (ucontext_t*)malloc(sizeof(ucontext_t));
    ucCaller_  = (ucontext_t*)malloc(sizeof(ucontext_t));

    /* 获取初始化 ucontext 实例 */ 
    getcontext(uc_);
    getcontext(ucCaller_);

    stack_ = (char*)malloc(CO_STACK_SIZE * sizeof(char));
    memset(stack_, 0, CO_STACK_SIZE * sizeof(char));

    /* 设置协程的上下文信息 */
    uc_->uc_stack.ss_sp = stack_;
    uc_->uc_stack.ss_size = CO_STACK_SIZE * sizeof(char); //当前栈信息
    uc_->uc_link = uc_link_; // uc_link指向一个上下文，当当前上下文结束时，将返回执行该上下文  //实际表明，将这行注释掉，也没啥影响
    /* 指定协程被调度之后要执行的函数 */
    uintptr_t ptr = (uintptr_t)this;
    makecontext(uc_, (void(*)(void))coFuncExecu, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
}


/**
 * @brief 保存协程上下文
*/
void Coroutine::coYield() {
    coState_ = CO_SUSPEND;
    char dummy = '\0';
    //std::cout << "stack size: " << stack_ + CO_STACK_SIZE - &dummy << std::endl;
    swapcontext(uc_, ucCaller_);
}

/**
 * @brief 恢复协程上下文并执行（即被调度）
*/
void Coroutine::coResume() {
    switch(coState_) {
        case CO_READY:
            InitCoroutine();
            coState_ = CO_RUNNING;
            swapcontext(ucCaller_, uc_);
            break;
        case CO_SUSPEND:
            coState_ = CO_RUNNING;
            swapcontext(ucCaller_, uc_);
            break;
        default:
            break;
    }
}
