#include "Coroutine.h"
static int coroutine_count = 0;

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
void Coroutine::InitCoroutine(int i) {
    id = i;
    uc_     = (ucontext_t*)malloc(sizeof(ucontext_t));
    ucCaller_  = (ucontext_t*)malloc(sizeof(ucontext_t));

    /* 获取初始化 ucontext 实例 */ 
    getcontext(uc_);
    getcontext(ucCaller_);
    capacity_ = CO_STACK_SIZE * sizeof(char);

    stack_ = malloc(CO_STACK_SIZE * sizeof(char));
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
    usedSpace_ = (long long int)stack_ + capacity_ - (long long int)&dummy;
    checkSpace();
    swapcontext(uc_, ucCaller_);
}


/*
    检查当前协程空间是否充足
*/
void Coroutine::checkSpace()
{
    if(usedSpace_ >  USAGE_UPLIMITED * capacity_) ++round;
    else if(usedSpace_ < USAGE_DOWNLIMITED * capacity_) ++round;
    else {round = 0; return;}

    if(round >= UP_LIMITED && usedSpace_ >  USAGE_UPLIMITED * capacity_) {coState_ = CO_OVERLOAD;}
    else if(round >= UP_DOWN && usedSpace_ < USAGE_DOWNLIMITED * capacity_) {coState_ = CO_LIGHTLOAD;}
    else {}
}

/*
    获得当前协程分配到的空间
*/
int Coroutine::getCapacity()
{
    return this->capacity_;
}

/*
    返回协程状态
*/
CO_STATE Coroutine::getCoState()
{
    return coState_;
}

/*
    返回当前协程已经使用的空间大小
*/
int Coroutine::getSpace()
{
    return this->usedSpace_;
}

/*
    增大协程内部空间
*/
void Coroutine::increaseSpace()
{
    int newSize = capacity_ * INCREASE_RATIO * sizeof(char);
    void* newStack_ = malloc(newSize);
    // 先进行内容篡改
    void* oldBottemStack_ = (void*)((char*)stack_ + capacity_);
    long long int delta = (long long int)newStack_ + newSize - (long long int)stack_ - capacity_; 

    //更改原栈中内容
    for(greg_t i = 0;i<=usedSpace_;i += sizeof(uintptr_t))
    {
        greg_t* ptr = (greg_t*)((long long int)oldBottemStack_ - i);
        if(*ptr >= (long long int)(oldBottemStack_) - usedSpace_ && *ptr <= (long long int)oldBottemStack_)
        {
            *ptr = (long long int)(*ptr) + delta;
        }
    }
    

    // 做内容迁移
    memcpy((char*)newStack_ + newSize - usedSpace_,(char*)stack_ + capacity_ - usedSpace_,usedSpace_);

    //做指针迁移
    uc_->uc_mcontext.gregs[REG_RSP] = uc_->uc_mcontext.gregs[REG_RSP] + delta;
    uc_->uc_mcontext.gregs[REG_RBP] = uc_->uc_mcontext.gregs[REG_RBP] + delta;
    free(stack_);
    capacity_ = newSize;
    stack_ = newStack_;

    //uc_->uc_stack.ss_sp = stack_;
    //uc_->uc_stack.ss_size = capacity_;
    round = 0;
    coState_ = CO_SUSPEND;
    
}

/*
    减小协程内部空间
*/
void Coroutine::shrinkSpace()
{
    int newSize = capacity_ * SHRINK_RATIO * sizeof(char);
    void* newStack_ = malloc(newSize);
    // 先进行内容篡改
    void* oldBottemStack_ = (void*)((char*)stack_ + capacity_);
    long long int delta = (long long int)newStack_ + newSize - (long long int)stack_ - capacity_; 

    //更改原栈中内容
    for(greg_t i = 0;i<=usedSpace_;i += sizeof(uintptr_t))
    {
        greg_t* ptr = (greg_t*)((long long int)oldBottemStack_ - i);
        if(*ptr >= (long long int)(oldBottemStack_) - usedSpace_ && *ptr <= (long long int)oldBottemStack_)
        {
            *ptr = (long long int)(*ptr) + delta;
        }
    }
    

    // 做内容迁移
    memcpy((char*)newStack_ + newSize - usedSpace_,(char*)stack_ + capacity_ - usedSpace_,usedSpace_);

    //做指针迁移
    uc_->uc_mcontext.gregs[REG_RSP] = uc_->uc_mcontext.gregs[REG_RSP] + delta;
    uc_->uc_mcontext.gregs[REG_RBP] = uc_->uc_mcontext.gregs[REG_RBP] + delta;
    free(stack_);
    capacity_ = newSize;
    stack_ = newStack_;

    //uc_->uc_stack.ss_sp = stack_;
    //uc_->uc_stack.ss_size = capacity_;
    round = 0;
    coState_ = CO_SUSPEND;
}

/**
 * @brief 恢复协程上下文并执行（即被调度）
*/
void Coroutine::coResume() {
    switch(coState_) {
        case CO_READY:
            ++coroutine_count;
            InitCoroutine(coroutine_count);
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
