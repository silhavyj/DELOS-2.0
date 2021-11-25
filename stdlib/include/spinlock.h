#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

// https://wiki.osdev.org/Spinlock

typedef unsigned int spinlock_t;

extern "C" {
    void spinlock_init(spinlock_t *lock);
    void spinlock_acquire(spinlock_t *lock);
    void spinlock_release(spinlock_t *lock);
}

#endif