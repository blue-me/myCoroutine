#include "Scheduler.h"
#include <mutex>
#include <thread>
/**
 * @brief 为调度器增加协程
*/
void Scheduler::addCoroutine(std::shared_ptr<Coroutine> && uc) {
    coVec_.emplace_back(std::move(uc));
}

/*
 * @brief 主线程应该完成协程的切换、唤醒、检查空间并标注工作
*/
void Scheduler::mainThread()
{
    schedule();
    mainThreadExited_ = true;
    cv_.notify_one();
}
/*
    次线程应该完成协程的空间再分配工作
*/
void Scheduler::spaceThread()
{
    while(true)
    {
        //如果主线程中止运行，则退出（避免主线程此时也退出）
        if(mainThreadExited_) break;

        std::unique_lock<std::mutex> lock(queueMutex_);
        cv_.wait(lock,[this]{return !spaceQueue_.empty() || mainThreadExited_; });
        
        //如果主线程中止运行，则退出
        if(mainThreadExited_) break;

        //一直消费队列，直至队列为空
        while(!spaceQueue_.empty())
        {
            std::shared_ptr<Coroutine> dealCoroutine = spaceQueue_.front();
            spaceQueue_.pop();

            if(dealCoroutine->getCoState() == CO_OVERLOAD_WORKING)
            {
                dealCoroutine->increaseSpace();
            }
            else if(dealCoroutine->getCoState() == CO_LIGHTLOAD_WORKING)
            {
                dealCoroutine->shrinkSpace();
            }
            else 
            {
                std::cout<<"Coroutine state is normal. It should not happen. "
                    <<dealCoroutine->getCoState()<<std::endl;
            }
            dealCoroutine->changeCoState(CO_SUSPEND);
        }
    }
}

/**
 * @brief 根据RR策略调度协程
*/
void Scheduler::schedule() {
    while(coVec_.size()) {
        for(auto iter = coVec_.begin(); iter != coVec_.end();) { // 避免迭代器失效问题
            if((*iter)->isCoOnTerm()) {
                iter = coVec_.erase(iter);
            } else {
                if((*iter)->getCoState() == CO_SUSPEND || (*iter)->getCoState() == CO_READY)  (*iter)->coResume();

                if((*iter)->getCoState() == CO_LIGHTLOAD || (*iter)->getCoState() == CO_OVERLOAD)
                {
                    if((*iter)->getCoState() == CO_LIGHTLOAD) (*iter)->changeCoState(CO_LIGHTLOAD_WORKING);
                    else (*iter)->changeCoState(CO_OVERLOAD_WORKING);
                    std::shared_ptr<Coroutine> copiedptr = *iter;
                    spaceQueue_.push(copiedptr);
                    cv_.notify_one();
                }
                ++iter;
            }
        }
    }
}

/*
    负责启动调度器的所有线程及回收完成
*/
void Scheduler::run()
{
    std::thread spaceT(&Scheduler::mainThread,this);
    std::thread mainT(&Scheduler::spaceThread,this);
    mainT.join();
    spaceT.join();
}