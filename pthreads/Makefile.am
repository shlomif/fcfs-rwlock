SUBDIRS = man

noinst_PROGRAMS = test_rwlock
test_rwlock_SOURCES = test_rwlock.c
test_rwlock_LDADD = libpthread_rwlock_fcfs.la

lib_LTLIBRARIES = libpthread_rwlock_fcfs.la
libpthread_rwlock_fcfsincludedir=$(includedir)/pthread
libpthread_rwlock_fcfs_la_SOURCES = rwlock.c queue.c
libpthread_rwlock_fcfs_la_LDFLAGS = -version-info 0:0:0
libpthread_rwlock_fcfsinclude_HEADERS = rwlock_fcfs.h rwlock_fcfs_queue.h

EXTRA_DIST += Docs/FCFS_RWLock_Scheme.txt Docs/FCFS_RWLock_Scheme_RLE.txt
EXTRA_DIST += test_rwlock.c 
EXTRA_DIST += test_queue.cpp

EXTRA_DIST += pthread/rwlock_fcfs.h pthread/rwlock_fcfs_queue.h

EXTRA_DIST += TODO


