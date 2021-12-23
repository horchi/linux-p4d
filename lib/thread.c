/*
 * thread.c:
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <linux/unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>

#include "thread.h"

//***************************************************************************
// get abs time plus 'millisecondsFromNow'
//***************************************************************************

static int absTime(struct timespec* abstime, int millisecondsFromNow)
{
  struct timeval now;

  if (gettimeofday(&now, 0) == 0)
  {
     // get current time

     now.tv_sec  += millisecondsFromNow / 1000;           // add full seconds
     now.tv_usec += (millisecondsFromNow % 1000) * 1000;  // add microseconds

     // take care of an overflow

     if (now.tv_usec >= 1000000)
     {
        now.tv_sec++;
        now.tv_usec -= 1000000;
     }

     abstime->tv_sec = now.tv_sec;          // seconds
     abstime->tv_nsec = now.tv_usec * 1000; // nano seconds

     return success;
  }

  return fail;
}

//***************************************************************************
// Class cThread
//***************************************************************************

cThread::cThread(const char* Description, bool LowPriority)
{
  active = running = no;
  childTid = 0;
  childThreadId = 0;
  description = 0;
  silent = no;

  if (Description)
     SetDescription("%s", Description);

  lowPriority = LowPriority;
  pthread_attr_init(&attr);
}

cThread::~cThread()
{
  Cancel(); // just in case the derived class didn't call it
  free(description);
  pthread_attr_destroy(&attr);
}

void cThread::SetDescription(const char *Description, ...)
{
  free(description);
  description = NULL;

  if (Description)
  {
     va_list ap;
     va_start(ap, Description);
     vasprintf(&description, Description, ap);
     va_end(ap);
  }
}

void *cThread::StartThread(cThread *Thread)
{
  Thread->childThreadId = ThreadId();
  if (Thread->description)
  {
     tell(Thread->silent ? eloDebug : eloAlways, "'%s' thread started (pid=%d, tid=%d, prio=%s)", Thread->description, getpid(), Thread->childThreadId, Thread->lowPriority ? "low" : "high");
#ifdef PR_SET_NAME
     if (prctl(PR_SET_NAME, Thread->description, 0, 0, 0) < 0)
        tell(eloAlways, "%s thread naming failed (pid=%d, tid=%d)", Thread->description, getpid(), Thread->childThreadId);
#endif
  }

  if (Thread->lowPriority)
  {
     Thread->SetPriority(19);
     Thread->SetIOPriority(7);
  }

  Thread->action();

  if (Thread->description)
     tell(Thread->silent ? eloDebug : eloAlways, "'%s' thread ended (pid=%d, tid=%d)", Thread->description, getpid(), Thread->childThreadId);

  Thread->running = false;
  Thread->active = false;

  return NULL;
}

//***************************************************************************
// Priority
//***************************************************************************

void cThread::SetPriority(int priority)
{
  if (setpriority(PRIO_PROCESS, 0, priority) < 0)
     tell(eloAlways, "Error: Setting priority failed");
}

void cThread::SetIOPriority(int priority)
{
  if (syscall(SYS_ioprio_set, 1, 0, (priority & 0xff) | (3 << 13)) < 0) // idle class
     tell(eloAlways, "Error: Setting io priority failed");
}

//***************************************************************************
// Start
//***************************************************************************

#define THREAD_STOP_TIMEOUT  3000 // ms to wait for a thread to stop before newly starting it
#define THREAD_STOP_SLEEP      30 // ms to sleep while waiting for a thread to stop

bool cThread::Start(int s, int stackSize)
{
  silent = s;

  if (!running)
  {
     if (active)
     {
        // Wait until the previous incarnation of this thread has completely ended
        // before starting it newly:

        cMyTimeMs RestartTimeout;

        while (!running && active && RestartTimeout.Elapsed() < THREAD_STOP_TIMEOUT)
              cCondWait::SleepMs(THREAD_STOP_SLEEP);
     }
     if (!active)
     {
        int res;

        active = running = true;

        if (stackSize == na)
        {
           res = pthread_create(&childTid, 0, (void*(*)(void*))&StartThread, (void*)this);
        }
        else
        {
           pthread_attr_setstacksize(&attr, stackSize);
           res = pthread_create(&childTid, &attr, (void*(*)(void*))&StartThread, (void*)this);
        }

        if (res == 0)
        {
           pthread_detach(childTid); // auto-reap
        }
        else
        {
           tell(eloAlways, "Error: Thread won't start");
           active = running = false;
           return false;
        }
     }
  }

  return true;
}

bool cThread::Active(void)
{
  if (active)
  {
     int err;

     if ((err = pthread_kill(childTid, 0)) != 0)
     {
        if (err != ESRCH)
           tell(eloAlways, "Error: Thread ...");
        childTid = 0;
        active = running = false;
     }
     else
        return true;
  }

  return false;
}

void cThread::Cancel(int WaitSeconds)
{
  running = false;

  waitCondition.Broadcast();

  if (active && WaitSeconds > -1)
  {
     if (WaitSeconds > 0)
     {
        for (time_t t0 = time(NULL) + WaitSeconds; time(NULL) < t0; )
        {
           if (!Active())
              return;
           cCondWait::SleepMs(10);
        }

        tell(eloAlways, "ERROR: %s thread %d won't end (waited %d seconds) - canceling it...", description ? description : "", childThreadId, WaitSeconds);
     }

     pthread_cancel(childTid);
     childTid = 0;
     active = false;
  }
}

pid_t cThread::ThreadId()
{
  return syscall(__NR_gettid);
}

//***************************************************************************
// cCondWait
//***************************************************************************

cCondWait::cCondWait()
{
   signaled = false;
   pthread_mutex_init(&mutex, NULL);
   pthread_cond_init(&cond, NULL);
}

cCondWait::~cCondWait()
{
   pthread_cond_broadcast(&cond); // wake up any sleepers
   pthread_cond_destroy(&cond);
   pthread_mutex_destroy(&mutex);
}

void cCondWait::SleepMs(int TimeoutMs)
{
   cCondWait w;
   w.Wait(std::max(TimeoutMs, 3)); // making sure the time is >2ms to avoid a possible busy wait
}

bool cCondWait::Wait(int TimeoutMs)
{
   pthread_mutex_lock(&mutex);

   if (!signaled)
   {
      if (TimeoutMs)
      {
         struct timespec abstime;

         if (absTime(&abstime, TimeoutMs) == success)
         {
            while (!signaled)
            {
               if (pthread_cond_timedwait(&cond, &mutex, &abstime) == ETIMEDOUT)
                  break;
            }
         }
      }
      else
         pthread_cond_wait(&cond, &mutex);
   }

   bool r = signaled;
   signaled = false;
   pthread_mutex_unlock(&mutex);

   return r;
}

void cCondWait::Signal()
{
   pthread_mutex_lock(&mutex);
   signaled = true;
   pthread_cond_broadcast(&cond);
   pthread_mutex_unlock(&mutex);
}

//***************************************************************************
// cCondVar
//***************************************************************************

cCondVar::cCondVar(void)
{
   pthread_cond_init(&cond, 0);
}

cCondVar::~cCondVar()
{
   pthread_cond_broadcast(&cond); // wake up any sleepers
   pthread_cond_destroy(&cond);
}

void cCondVar::Wait(cMyMutex &Mutex)
{
   if (Mutex.locked)
   {
      int locked = Mutex.locked;
      Mutex.locked = 0; // have to clear the locked count here, as pthread_cond_wait
      // does an implicit unlock of the mutex
      pthread_cond_wait(&cond, &Mutex.mutex);
      Mutex.locked = locked;
   }
}

bool cCondVar::TimedWait(cMyMutex &Mutex, int TimeoutMs)
{
   bool r = true;     // true = condition signaled, false = timeout

   if (Mutex.locked)
   {
      struct timespec abstime;

      if (absTime(&abstime, TimeoutMs) == success)
      {
         int locked = Mutex.locked;

         // have to clear the locked count here, as pthread_cond_timedwait
         // does an implicit unlock of the mutex.

         Mutex.locked = 0;

         if (pthread_cond_timedwait(&cond, &Mutex.mutex, &abstime) == ETIMEDOUT)
            r = false;

         Mutex.locked = locked;
     }
   }

   return r;
}

void cCondVar::Broadcast(void)
{
   pthread_cond_broadcast(&cond);
}
