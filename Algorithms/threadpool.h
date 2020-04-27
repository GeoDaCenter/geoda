#ifndef __THREAD_POOL_H___
#define __THREAD_POOL_H___

#include <stdio.h>
#include <queue>
#include <stdlib.h>

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/phoenix.hpp>
#include <boost/optional.hpp>
#include <boost/container/deque.hpp>

#include "../GdaConst.h"

typedef boost::function<void()> job_t;

class thread_pool
{
private:
    boost::mutex mx;
    boost::condition_variable cv;

    boost::container::deque<job_t> _queue;

    boost::thread_group pool;

    boost::atomic_bool shutdown;
    static void worker_thread(thread_pool& q)
    {
        while (boost::optional<job_t> job = q.dequeue())
            (*job)();
    }

public:
    thread_pool() : shutdown(false) {
        int cores = boost::thread::hardware_concurrency();
        if (GdaConst::gda_set_cpu_cores) cores = GdaConst::gda_cpu_cores;
        if (cores > 1) cores = cores -1;
        for (unsigned i = 0; i < cores; ++i)
            pool.create_thread(boost::bind(worker_thread, boost::ref(*this)));
    }

    void enqueue(job_t job)
    {
        boost::lock_guard<boost::mutex> lk(mx);
        _queue.push_back(job);

        cv.notify_one();
    }

    boost::optional<job_t> dequeue()
    {
        boost::unique_lock<boost::mutex> lk(mx);
        namespace phx = boost::phoenix;

        cv.wait(lk, phx::ref(shutdown) || !phx::empty(phx::ref(_queue)));

        if (_queue.empty())
            return boost::none;

        job_t job = _queue.front();
        _queue.pop_front();

        return job;
    }

    ~thread_pool()
    {
        shutdown = true;
        {
            boost::lock_guard<boost::mutex> lk(mx);
            cv.notify_all();
        }

        pool.join_all();
    }
};

#else

#include <pthread.h>

class Task {
public:
    Task() {}
    ~Task() {}
    virtual void run()=0;
    virtual void indicateTaken()=0;
};

class WorkQueue {
public:
    WorkQueue() {
        pthread_mutex_init(&qmtx,0);
        pthread_cond_init(&wcond, 0);
    }
    ~WorkQueue() {
        pthread_mutex_destroy(&qmtx);
        pthread_cond_destroy(&wcond);
    }
    Task *nextTask() {
        Task *nt;
        pthread_mutex_lock(&qmtx);
        if (finished && tasks.size() == 0) {
            nt = 0;
        } else {
            if (tasks.size()==0) {
                pthread_cond_wait(&wcond, &qmtx);
            }
            nt = tasks.front();
            tasks.pop();
        }
        pthread_mutex_unlock(&qmtx);
        return nt;
    }
    void addTask(Task *nt) {
        if (!finished) {
            pthread_mutex_lock(&qmtx);
            tasks.push(nt);
            pthread_cond_signal(&wcond);
            pthread_mutex_unlock(&qmtx);
        }
    }
    void finish() {
        pthread_mutex_lock(&qmtx);
        finished = true;
        pthread_cond_signal(&wcond);
        pthread_mutex_unlock(&qmtx);
    }
    bool hasWork() {
        return (tasks.size()>0);
    }

private:
    std::queue<Task*> tasks;
    bool finished;
    pthread_mutex_t qmtx;
    pthread_cond_t wcond;
};

static void *getWork(void* param) {
    Task *mw = 0;
    WorkQueue *wq = (WorkQueue*)param;
    while (mw = wq->nextTask()) {
        mw->indicateTaken();
        mw->run();
        delete mw;
    }
    return 0;
}

class ThreadPool {
public:
    ThreadPool(int n) : _numThreads(n) {
        printf("Creating a thread pool with %d threads\n", n);
        threads = new pthread_t[n];
        for (int i=0; i< n; ++i) {
            pthread_create(&(threads[i]), 0, getWork, &workQueue);
        }
    }

    ~ThreadPool() {
        workQueue.finish();
        for (int i=0; i<_numThreads; ++i) {
            pthread_join(threads[i], 0);
        }
        delete [] threads;
    }

    void addTask(Task *nt) {
        workQueue.addTask(nt);
    }
    void finish() {
        workQueue.finish();
    }
    bool hasWork() {
        return workQueue.hasWork();
    }
    void waitForCompletion() {
        while (workQueue.hasWork()) {}
    }

private:
    pthread_t *threads;
    int _numThreads;
    WorkQueue workQueue;
};

static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;
void showTask(int n) {
    pthread_mutex_lock(&console_mutex);
    printf("Adding fibonacci task %d\n", n);
    pthread_mutex_unlock(&console_mutex);
}

class FibTask : public Task {
public:
    FibTask(int n) : Task(), _n(n) {}
    ~FibTask() {
        pthread_mutex_lock(&console_mutex);
        printf("Fibonacci task %d being deleted\n", _n);
        pthread_mutex_unlock(&console_mutex);
    }
    virtual void run() {
        int val = innerFib(_n);
        pthread_mutex_lock(&console_mutex);
        printf("Fibd %d = %d\n",_n, val);
        pthread_mutex_unlock(&console_mutex);
    }
    virtual void indicateTaken() {
        pthread_mutex_lock(&console_mutex);
        printf("Took fibonacci task %d\n", _n);
        pthread_mutex_unlock(&console_mutex);
    }
private:
    int innerFib(int n) {
        if (n<=1) { return 1; }
        return innerFib(n-1) + innerFib(n-2);
    }
    int _n;
};

int test() {
    ThreadPool *tp = new ThreadPool(8);
    for (int i=0;i<30; ++i) {
        int rv = rand() % 30 + 9;
        showTask(rv);
        tp->addTask(new FibTask(rv));
    }
    tp->finish();
    tp->waitForCompletion();
    delete tp;
    printf("Done with all work!\n");
}

#endif
