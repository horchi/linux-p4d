/*
 * thread.h
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include <pthread.h>

#include "common.h"

class cCondWait
{
   public:

      cCondWait();
      ~cCondWait();

      bool Wait(int TimeoutMs = 0);
      void Signal();

      static void SleepMs(int TimeoutMs);

   private:

      pthread_mutex_t mutex;
      pthread_cond_t cond;
      bool signaled;
};

class cCondVar
{
   public:

      cCondVar();
      ~cCondVar();

      void Wait(cMyMutex &Mutex);
      bool TimedWait(cMyMutex &Mutex, int TimeoutMs);
      void Broadcast();

   private:

      pthread_cond_t cond;
};

//***************************************************************************
// Class cThread
//***************************************************************************

class cThread
{
   public:

      cThread(const char *Description = NULL, bool LowPriority = false);
      virtual ~cThread();

      void SetDescription(const char* Description, ...) __attribute__ ((format (printf, 2, 3)));
      bool Start(int s = no, int stackSize = na);
      bool Active();
      void SetPriority(int priority);
      void SetIOPriority(int priority);
      void Cancel(int WaitSeconds = 0);

      static pid_t ThreadId();

   private:

      static void* StartThread(cThread *Thread);

      pthread_t childTid;
      pid_t childThreadId;
      char* description;
      bool lowPriority;
      int silent;
      pthread_attr_t attr;

   protected:

      virtual void action() = 0;

      void Lock()          { mutex.Lock(); }
      void Unlock()        { mutex.Unlock(); }
      bool Running()       { return running; }

      cCondVar waitCondition;
      cMyMutex mutex;
      bool active;
      bool running;
};
