#pragma once
#include <cstdlib>
#include <pthread.h>

class mylock{
public:
    mylock()
    {
        int n = pthread_mutex_init(&_lock,nullptr);
        if(n != 0)
        {
            exit(1);
        }
    }
    int lock()
    {
        return pthread_mutex_lock(&_lock);
    }
    int unlock()
    {
        return pthread_mutex_unlock(&_lock);
    }
    pthread_mutex_t* get_lock()
    {
        return &_lock;
    }
    ~mylock()
    {
        pthread_mutex_destroy(&_lock);
    }
private:
    pthread_mutex_t _lock;
};
