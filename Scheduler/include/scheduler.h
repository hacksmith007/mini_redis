//
// Created by Rahul Ranjan on 13/04/26.
//

#ifndef MINI_REDIS_SCHEDULER_H
#define MINI_REDIS_SCHEDULER_H
#pragma once

#include <functional>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    // Register a task with interval in milliseconds
    void register_task(std::function<void()> func, int interval_ms);

    // Stop scheduler
    void stop();

private:
    struct Task {
        std::function<void()> func;
        std::chrono::milliseconds interval;
        std::chrono::steady_clock::time_point next_run;
    };

    std::vector<Task> tasks;

    std::atomic<bool> running;
    std::thread worker;
    std::mutex mtx;

    void run();  // worker loop
};

void expiryPoll();
#endif //MINI_REDIS_SCHEDULER_H
