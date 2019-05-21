// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COIN_MESSAGEQUEUE_H
#define COIN_MESSAGEQUEUE_H

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

constexpr std::chrono::milliseconds POP_DEFAULT_TIMEOUT{20};
constexpr size_t MSG_QUEUE_DEFAULT_MAX_LEN = 10000;
constexpr size_t MSG_QUEUE_MAX_LEN         = 60000;

template <typename T>
class MsgQueue final {
public:
    using SizeType = typename std::queue<T>::size_type;
    using Timeout  = std::chrono::milliseconds;

public:
    MsgQueue(const SizeType maxLen = MSG_QUEUE_DEFAULT_MAX_LEN) : mqMaxLen(maxLen) {}

public:
    bool Pop(T* t = nullptr, const Timeout& timeout = POP_DEFAULT_TIMEOUT);
    void Push(const T& t);
    void Push(T&& t);

public:
    bool Empty();
    bool Full();
    SizeType Len();

private:
    std::queue<T> mq;
    SizeType mqMaxLen;
    std::condition_variable popCond;
    std::condition_variable pushCond;
    std::mutex mtx;
};

template <typename T>
bool MsgQueue<T>::Pop(T* t, const Timeout& timeout) {
    std::unique_lock<std::mutex> lock(mtx);

    if (mq.empty()) {
        // `wait_for' will return after popCond has been notified or
        // specious wake-up happens or times out.
        // so `Pop' may return before times out though popCond is not notified
        popCond.wait_for(lock, timeout);
    }

    if (!mq.empty()) {
        if (mq.size() == mqMaxLen) {
            pushCond.notify_all();
        }
        if (t) {
            *t = std::move(mq.front());
        }
        mq.pop();
        return true;
    }

    return false;
}

template <typename T>
void MsgQueue<T>::Push(const T& t) {
    Push(T(t));
}

template <typename T>
void MsgQueue<T>::Push(T&& t) {
    std::unique_lock<std::mutex> lock(mtx);

    while (mq.size() == mqMaxLen) {
        pushCond.wait(lock);
    }
    if (mq.empty()) {
        popCond.notify_all();
    }
    mq.emplace(std::move(t));
}

template <typename T>
bool MsgQueue<T>::Empty() {
    std::unique_lock<std::mutex> lock(mtx);
    return mq.empty();
}

template <typename T>
bool MsgQueue<T>::Full() {
    std::unique_lock<std::mutex> lock(mtx);
    return mq.size() == mqMaxLen;
}

template <typename T>
typename MsgQueue<T>::SizeType MsgQueue<T>::Len() {
    std::unique_lock<std::mutex> lock(mtx);
    return mq.size();
}

#endif  // COIN_MESSAGEQUEUE_H