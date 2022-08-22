
#include "devfs_os.h"

void* devfs_malloc(size_t size)
{
    return k_malloc(size);
}

void devfs_free(void *mem)
{
    if (NULL != mem)
        k_free(mem);
}

int devfs_mutex_init(devfs_mutex_t *mutex)
{
    return k_mutex_init(mutex);
}

int devfs_mutex_lock(devfs_mutex_t *mutex, unsigned int timeout)
{
    return k_mutex_lock(mutex, ((timeout == DEVFS_FOREVER) ? K_FOREVER : Z_TIMEOUT_MS(timeout)));
}

int devfs_mutex_unlock(devfs_mutex_t *mutex)
{
    return k_mutex_unlock(mutex);
}

int devfs_mutex_free(devfs_mutex_t *mutex)
{
    /* do nothing */

    return 0;
}

int devfs_sem_init(devfs_sem_t *sem, unsigned int initial, unsigned int limit)
{
    return k_sem_init(sem, initial, limit);
}

int devfs_sem_take(devfs_sem_t *sem, unsigned int timeout)
{
    return k_sem_take(sem, ((timeout == DEVFS_FOREVER) ? K_FOREVER : Z_TIMEOUT_MS(timeout)));
}

int devfs_sem_give(devfs_sem_t *sem)
{
    k_sem_give(sem);

    return 0;
}

int devfs_sem_free(devfs_sem_t *sem)
{
    /* do nothing */

    return 0;
}