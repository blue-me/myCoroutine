#pragma once
#include<vector>
#include<memory>
#include<queue>
#include<atomic>
#include<condition_variable>
#include<mutex>
#include"Coroutine.h"

#define MAX_COROUTINE 1024

/**
 * @brief 一个基于Round-Robin策略的协程调度器
*/
class Scheduler {
public:
    Scheduler() {}
    ~Scheduler() {}

    void addCoroutine(std::shared_ptr<Coroutine> &&);
    void run();
    void mainThread();
    void spaceThread();

private:
    void schedule();

private:
    /* 调度器可调度的协程列表 */
    std::vector<std::shared_ptr<Coroutine>> coVec_;
    //ucontext_t*                             uc_link_; //这个其实没用，在此处公共栈中的stack已经下放到每个协程中，每个协程中的这些栈负责执行Scheduler->Coroutine COuroutine->Scheduler的路径
    std::queue<std::shared_ptr<Coroutine>> spaceQueue_;
    std::atomic<bool> mainThreadExited_{false};
    std::condition_variable cv_; //条件变量，线程间同步
    std::mutex queueMutex_; //互斥锁，保护spaceQueue_的访问

};