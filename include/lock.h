#ifndef _FRUL_LOCK_H
#define _FRUL_LOCK_H

#include <stdatomic.h>

typedef volatile atomic_flag spinlock_t;

static inline void spinlock_lock(spinlock_t *lk)
{
  while (atomic_flag_test_and_set(lk));
}

static inline void spinlock_unlock(spinlock_t *lk)
{
  atomic_flag_clear(lk);
}

#endif