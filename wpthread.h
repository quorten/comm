/* A lightweight and incomplete pthreads to Windows threads wrapper.

Public Domain 2012, 2020 Andrew Makousky

See the file "UNLICENSE" in the top level directory for details.

*/
#ifndef WPTHREAD_H
#define WPTHREAD_H

#define pthread_t DWORD
#define pthread_mutex_t HANDLE
#define PTHREAD_MUTEX_INITIALIZER INVALID_HANDLE

#define pthread_self() GetCurrentThreadId()
#define pthread_mutex_lock(mutex) WaitForSingleObject(*(mutex), INFINITE)
#define pthread_equal(p1, p2) (p1 == p2)
#define pthread_mutex_unlock(mutex) ReleaseMutex(*(mutex))
#define pthread_exit(val) ExitThread((DWORD)(val))
#define pthread_mutex_destroy(mutex) CloseHandle(*(mutex))
#define pthread_create(thread_id, props, proc, user_data) \
  (CreateThread(NULL, 0, proc, user_data, thread_id) == NULL)
#define pthread_mutex_init(mutex, attr) \
  ((*(mutex)) = CreateMutex(NULL, FALSE, NULL))

#endif /* not WPTHREAD_H */
