//
// Created by Rahul Ranjan on 13/04/26.
//

#include <iostream>
#include "scheduler.h"

#include "RedisCommon.h"
#include  "store.h"

/**
 * ============================================================
 * FUNCTION: Scheduler (constructor)
 * ============================================================
 * Initializes scheduler and starts worker thread.
 * ============================================================
 */
Scheduler::Scheduler() : running(true) {
    worker = std::thread(&Scheduler::run, this);
}

/**
 * ============================================================
 * FUNCTION: ~Scheduler (destructor)
 * ============================================================
 * Ensures scheduler is stopped and worker thread is joined.
 * ============================================================
 */
Scheduler::~Scheduler() {
    stop();
}

/**
 * ============================================================
 * FUNCTION: register_task
 * ============================================================
 * Registers a new periodic task with specified interval.
 * Thread-safe insertion into task list.
 * ============================================================
 */
void Scheduler::register_task(std::function<void()> func, int interval_ms) {
    std::lock_guard<std::mutex> lock(mtx);

    tasks.push_back({
        func,
        std::chrono::milliseconds(interval_ms),
        std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms)
    });
}

/**
 * ============================================================
 * FUNCTION: run
 * ============================================================
 * Worker loop that continuously checks and executes tasks
 * whose scheduled time has arrived.
 * ============================================================
 */
void Scheduler::run() {
    while (running) {
        auto now = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(mtx);

            for (auto &task : tasks) {
                if (now >= task.next_run) {
                    try {
                        task.func(); // execute task
                    } catch (...) {
                        std::cerr << "Task threw exception\n";
                    }

                    task.next_run = now + task.interval;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // prevent busy spinning
    }
}

/**
 * ============================================================
 * FUNCTION: stop
 * ============================================================
 * Stops scheduler loop and joins worker thread safely.
 * ============================================================
 */
void Scheduler::stop() {
    running = false;

    if (worker.joinable()) {
        worker.join();
    }
}

/**
 * ============================================================
 * FUNCTION: expiryPoll
 * ============================================================
 * Example periodic task function for cacheExpirtyDb handling.
 * Intended to be scheduled via Scheduler.
 * ============================================================
 */
void expiryPoll(Store &store) {
    // REDIS_LOG(INFO, "ExpiryPoll Running");
    store.redisCleanupExpired();
}