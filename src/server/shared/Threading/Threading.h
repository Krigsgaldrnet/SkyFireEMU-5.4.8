/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef THREADING_H
#define THREADING_H

#include <atomic>
#include <ace/Thread.h>
#include <ace/TSS_T.h>
#include <assert.h>

namespace ACE_Based
{

    class Runnable
    {
    public:
        virtual ~Runnable() { }
        virtual void run() = 0;

        void incReference() { ++m_refs; }
        void decReference()
        {
            if (!--m_refs)
                delete this;
        }
    private:
        std::atomic<long> m_refs;
        //ACE_Atomic_Op<ACE_Thread_Mutex, long> m_refs;
    };

    enum Priority
    {
        Idle,
        Lowest,
        Low,
        Normal,
        High,
        Highest,
        Realtime
    };

#define MAXPRIORITYNUM (Realtime + 1)

    class ThreadPriority
    {
    public:
        ThreadPriority();
        int getPriority(Priority p) const;

    private:
        int m_priority[MAXPRIORITYNUM];
    };

    class Thread
    {
    public:
        Thread();
        explicit Thread(Runnable* instance);
        ~Thread();

        bool start();
        bool wait();
        void destroy();

        void suspend();
        void resume();

        void setPriority(Priority type);

        static void Sleep(unsigned long msecs);
        static ACE_thread_t currentId();
        static ACE_hthread_t currentHandle();
        static Thread* current();

    private:
        Thread(const Thread&);
        Thread& operator=(const Thread&);

        static ACE_THR_FUNC_RETURN ThreadTask(void* param);

        ACE_thread_t m_iThreadId;
        ACE_hthread_t m_hThreadHandle;
        Runnable* m_task;

        typedef ACE_TSS<Thread> ThreadStorage;
        //global object - container for Thread class representation of every thread
        static ThreadStorage m_ThreadStorage;
        //use this object to determine current OS thread priority values mapped to enum Priority{ }
        static ThreadPriority m_TpEnum;
    };

}
#endif
