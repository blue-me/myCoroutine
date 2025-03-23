#include "Scheduler.h"

/**
 * @brief 为调度器增加协程
*/
void Scheduler::addCoroutine(std::unique_ptr<Coroutine> && uc) {
    coVec_.emplace_back(std::move(uc));
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
                (*iter)->coResume();
                ++iter;
            }
        }
    }
}