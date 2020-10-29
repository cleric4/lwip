/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
// scmRTOS port implementation - Andrey V. Chuikin
#include    <arch/cc.h>
#include    <lwip/sys.h>
#include    <lwip/stats.h>
#include    <lwport_config.h>
#include    <scmRTOS.h>
#include "pmc_loader.h"

//lwip_proc LwIP_proc(TCPIP_THREAD_NAME);
struct semaphore : public OS::TEventFlag
{
//    semaphore(OS::TEventFlag::TValue value) : OS::TEventFlag(value) {}
};

struct mutex : public OS::TMutex
{
};

struct mailbox : public OS::channel<void*, ARCH_QUEUE_SIZE>
{
};

namespace ethernet
{
    //------------------------------------------------------------------------------
    //
    // CLASS: Pool
    //
    // DESCRIPTION:
    // Пул заданных объектов.
    // Самый простой (но не самый быстрый) вариант.
    //
    template <class T, size_t N>
    class Pool
    {
    public:
        Pool() {}
       ~Pool() {}
       
        // Выделяет один объект из пула. Возвращает указатель на него
        // или 0, если нет места.
        T* alloc()
        {
            for (size_t i=0; i<N; ++i)
            {
                if (!pool[i].used)
                {
                    pool[i].used = true;
                    return &pool[i].obj;
                }
            }
            return 0;
        }
        
        // Возвращает указанный объект в пул.
        void free(T* obj)
        {
            // Если объекта нет среди выданных, то выдаем диагностику,
            // но ничего не делаем.
            for (size_t i=0; i<N; ++i)
            {
                if (obj == &pool[i].obj)
                {
                    pool[i].used = false;
                    return;
                }
            }
            LWIP_DEBUGF(LWIP_DBG_LEVEL_SERIOUS, ("Attempt to free already deallocated object\n"));
        }
        
    private:
        // В пуле сохраняются эти объекты.
        struct Item
        {
            T obj;       // Пользовательский объект пула.
            bool used;   // Признак использования.
               
            Item(): obj(), used(false) {}
        };
            
    private:
        Item pool[N];
    };

    Pool<semaphore, ARCH_SEM_POOL_SIZE> Semaphore_pool;
    Pool<mutex, ARCH_MUTEX_POOL_SIZE> Mutex_pool;
    Pool<mailbox, ARCH_QUEUE_POOL_SIZE> Mbox_pool;
} // ns
using namespace ethernet;

extern "C"
{
void sys_init() {}
//------------------------------------------------------------------------------
// Check if a mutex/sem/mbox is valid/allocated: return 1 for valid, 0 for invalid

int sys_sem_valid(sys_sem_t* sem)
{
    return (sem && *sem != 0);
}

void sys_sem_set_invalid(sys_sem_t* sem)
{
    if (sem) *sem = 0;
}

int sys_mutex_valid(sys_mutex_t* mutex)
{
    return (mutex && *mutex != 0);
}

void sys_mutex_set_invalid(sys_mutex_t* mutex)
{
    if (mutex) *mutex = 0;
}

int sys_mbox_valid(sys_mbox_t* mbox)
{
    return (mbox && *mbox != 0);
}
void sys_mbox_set_invalid(sys_mbox_t* mbox)
{
    if (mbox) *mbox = 0;
}

//------------------------------------------------------------------------------
u32_t sys_now(void)
{
    return OS::get_tick_count();
}

u32_t sys_jiffies(void)
{
    return sys_now() / RTOS_MS(10);
}

void os_sleep(timeout_t time)
{
    if(time)
    {
        if(RTOS_MS(1) != 1)
            time = RTOS_MS(time);
        OS::sleep(time);
    }
}
//------------------------------------------------------------------------------
//  This optional function does a "fast" critical region protection and returns
//  the previous protection level. This function is only called during very short
//  critical regions. An embedded system which supports ISR-based drivers might
//  want to implement this function by disabling interrupts. Task-based systems
//  might want to implement this by using a mutex or disabling tasking. This
//  function should support recursive calls from the same task or interrupt. In
//  other words, sys_arch_protect() could be called while already protected. In
//  that case the return value indicates that it is already protected.
//
//  sys_arch_protect() is only required if your port is supporting an operating
//  system.
sys_prot_t sys_arch_protect(void)
{
    sys_prot_t s = get_interrupt_state();
    disable_interrupts();
    return s;
}

//------------------------------------------------------------------------------
//  This optional function does a "fast" set of critical region protection to the
//  value specified by pval. See the documentation for sys_arch_protect() for
//  more information. This function is only required if your port is supporting
//  an operating system.
void sys_arch_unprotect(sys_prot_t pval)
{
    set_interrupt_state(pval);
}

//------------------------------------------------------------------------------
// Creates a new semaphore, returns it through the sem pointer provided as 
// argument to the function, in addition the function returns ERR_MEM if a new
// semaphore could not be created and ERR_OK if the semaphore was created.
// The count argument specifies the initial state of the semaphore.
err_t sys_sem_new(sys_sem_t* sem, u8_t count)
{
    LWIP_ASSERT("sem != NULL", sem != 0);
    
    auto t = Semaphore_pool.alloc();

    if (0 == t)
    {
        SYS_STATS_INC(sem.err);
        return ERR_MEM;
    }

    if (count)
        t->signal();

    SYS_STATS_INC_USED(sem);

    *sem = t;
    return ERR_OK;

}

//------------------------------------------------------------------------------
// Frees a semaphore created by sys_sem_new. Since these two functions provide
// the entry and exit point for all semaphores used by lwIP, you have great 
// flexibility in how these are allocated and deallocated (for example, from the
// heap, a memory pool, a semaphore pool, etc)
void sys_sem_free(sys_sem_t* sem)
{
    LWIP_ASSERT("sem != NULL", sem != 0);
    LWIP_ASSERT("*sem != NULL", *sem != 0);
    
    (*sem)->clear();
    Semaphore_pool.free(*sem);
    SYS_STATS_DEC(sem.used);
}

//------------------------------------------------------------------------------
// Signals (or releases) a semaphore referenced by *sem.
void sys_sem_signal(sys_sem_t* sem)
{
    LWIP_ASSERT("sem != NULL", sem != 0);
    LWIP_ASSERT("*sem != NULL", *sem != 0);
    (*sem)->signal();
}

//------------------------------------------------------------------------------
// Blocks the thread while waiting for the semaphore to be signaled. 
// The timeout parameter specifies how many milliseconds the function should 
// block before returning; if the function times out, it should return
// SYS_ARCH_TIMEOUT. If timeout=0, then the function should block indefinitely.
// If the function acquires the semaphore, it should return how many milliseconds
// expired while waiting for the semaphore. The function may return 0 if the 
// semaphore was immediately available.
uint32_t sys_arch_sem_wait(sys_sem_t* sem, u32_t timeout)
{
    LWIP_ASSERT("sem != NULL", sem != 0);
    LWIP_ASSERT("*sem != NULL", *sem != 0);
    tick_count_t start_time = sys_now();
    if(!(*sem)->wait(RTOS_MS_Z(timeout)))
        return SYS_ARCH_TIMEOUT;
    return RTOS_MS_Z(sys_now() - start_time);
}
  
//------------------------------------------------------------------------------
// Tries to create a new mailbox and return it via the mbox pointer provided as
// argument to the function. Returns ERR_OK if a mailbox was created and ERR_MEM
// if the mailbox on error.
err_t sys_mbox_new(sys_mbox_t* mbox, int size __attribute__((unused)))
{
    LWIP_ASSERT("mbox != NULL", mbox != 0);
    
    auto t = Mbox_pool.alloc();
    
    if (0 == t)
    {
        SYS_STATS_INC(mbox.err);
        return ERR_MEM;
    }
    
    SYS_STATS_INC_USED(mbox);
    
    *mbox = t;
    return ERR_OK;
}

//------------------------------------------------------------------------------
// Deallocates a mailbox. If there are messages still present in the mailbox
// when the mailbox is deallocated, it is an indication of a programming error 
// in lwIP and the developer should be notified.
void sys_mbox_free(sys_mbox_t* mbox)
{
    LWIP_ASSERT("mbox != NULL", mbox != 0);
    LWIP_ASSERT("*mbox != NULL", *mbox != 0);
    
    auto t = *mbox;
    if (t->get_count())
    {
        t->flush();
        LWIP_DEBUGF(LWIP_DBG_LEVEL_SERIOUS , ("Deleted mailbox is not empty\n"));
    }
    Mbox_pool.free(t);
    SYS_STATS_DEC(mbox.used);
}

//------------------------------------------------------------------------------
// Posts the "msg" to the mailbox.
void sys_mbox_post(sys_mbox_t* mbox, void* msg)
{
    LWIP_ASSERT("mbox != NULL", mbox != 0);
    LWIP_ASSERT("*mbox != NULL", *mbox != 0);
    
    (*mbox)->push(msg);
}

//------------------------------------------------------------------------------
// Tries to post a message to mbox by polling (no timeout).
// The function returns ERR_OK on success and ERR_MEM if it can't post at the moment.
err_t sys_mbox_trypost(sys_mbox_t* mbox, void* msg)
{
    LWIP_ASSERT("mbox != NULL", mbox != 0);
    LWIP_ASSERT("*mbox != NULL", *mbox != 0);
    
    auto t = *mbox;
    if (t->get_free_size())
    {
        t->push(msg);
        return ERR_OK;
    }
    
    return ERR_MEM;
}

//------------------------------------------------------------------------------
// Blocks the thread until a message arrives in the mailbox, but does not block
// the thread longer than timeout milliseconds (similar to the sys_arch_sem_wait()
// function). The msg argument is a pointer to the message in the mailbox and 
// may be NULL to indicate that the message should be dropped. This should 
// return either SYS_ARCH_TIMEOUT or the number of milliseconds elapsed waiting 
// for a message.
uint32_t sys_arch_mbox_fetch(sys_mbox_t* mbox, void** msg, u32_t timeout)
{
    LWIP_ASSERT("mbox != NULL", mbox != 0);
    LWIP_ASSERT("*mbox != NULL", *mbox != 0);
    
    void* dummyptr;
    if (0 == msg)
        msg = &dummyptr;

    tick_count_t start_time = sys_now();
    if(!(*mbox)->pop(*msg, RTOS_MS_Z(timeout)))
        return SYS_ARCH_TIMEOUT;
    return RTOS_MS_Z(sys_now() - start_time);
}

//------------------------------------------------------------------------------
// This is similar to sys_arch_mbox_fetch, however if a message is not present 
// in the mailbox, it immediately returns with the code SYS_MBOX_EMPTY. 
// On success ERR_OK is returned with msg pointing to the message retrieved from 
// the mailbox.
uint32_t sys_arch_mbox_tryfetch(sys_mbox_t* mbox, void** msg)
{
    LWIP_ASSERT("mbox != NULL", mbox != 0);
    LWIP_ASSERT("*mbox != NULL", *mbox != 0);

    if ((*mbox)->get_count() == 0)
        return SYS_MBOX_EMPTY;

    void* dummyptr;
    if (0 == msg)
        msg = &dummyptr;
    (*mbox)->pop(*msg);
    return ERR_OK;
}

//------------------------------------------------------------------------------
// Creates a new mutex, returns it through the mutex pointer provided as 
// argument to the function, in addition the function returns ERR_MEM if a new
// mutex could not be created and ERR_OK if the mutex was created.
err_t sys_mutex_new(sys_mutex_t* mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != 0);
    auto * t = Mutex_pool.alloc();

    if (0 == t)
    {
        SYS_STATS_INC(mutex.err);
        return ERR_MEM;
    }

    SYS_STATS_INC_USED(mutex);
    *mutex = t;
    return ERR_OK;
}

//------------------------------------------------------------------------------
// Lock a mutex
void sys_mutex_lock(sys_mutex_t* mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != 0);
    LWIP_ASSERT("*mutex != NULL", *mutex != 0);
    
    (*mutex)->lock();
}

//------------------------------------------------------------------------------
// Unlock a mutex
void sys_mutex_unlock(sys_mutex_t* mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != 0);
    LWIP_ASSERT("*mutex != NULL", *mutex != 0);
    
    (*mutex)->unlock();
}

//------------------------------------------------------------------------------
// Delete a mutex
void sys_mutex_free(sys_mutex_t* mutex)
{
    LWIP_ASSERT("mutex != NULL", mutex != 0);
    LWIP_ASSERT("*mutex != NULL", *mutex != 0);
    

    if ((*mutex)->is_locked())
    {
        LWIP_DEBUGF(LWIP_DBG_LEVEL_SERIOUS, ("Deleted mutex is not unlocked\n"));
        (*mutex)->unlock_isr();
    }
    Mutex_pool.free(*mutex);
    SYS_STATS_DEC(mutex.used);
}

//------------------------------------------------------------------------------
// name is the thread name. thread(arg) is the call made at the thread's entry
// point. stacksize is the recommanded stack size for this thread. prio is the
// priority that lwIP asks for. Stack size(s) and priority(ies) have to be are 
// defined in lwipopts.h, and so are completely customizable for your 
// system.

struct job_descriptor
{
    lwip_thread_fn  Thread;
    void *          Arg;
}  static  volatile Job;

sys_thread_t sys_thread_new(const char *name __attribute__((__unused__)),
        lwip_thread_fn thread, void *arg,
        int stacksize __attribute__((__unused__)), int prio __attribute__((__unused__)))
{
    Job.Arg = arg;
    Job.Thread = thread;
    return 0;
}

}   // extern "C"

//------------------------------------------------------------------------------
namespace OS
{
    template<>
    OS_PROCESS void lwip_proc::exec()
    {
        lwip_thread_fn Thread;
        while(!(Thread = Job.Thread))
            OS::sleep(RTOS_MS(100));

        Thread(Job.Arg);

        __builtin_unreachable();
    }
    lwip_proc LwIP_proc(TCPIP_THREAD_NAME);
}


