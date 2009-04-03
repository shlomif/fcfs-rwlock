man_MANS = pthread_rwlock_fcfs.3thr  \
    pthread_rwlock_fcfs_alloc.3thr \
    pthread_rwlock_fcfs_timed_gain_read.3thr \
    pthread_rwlock_fcfs_timed_gain_write.3thr \
    pthread_rwlock_fcfs_try_gain_read.3thr \
    pthread_rwlock_fcfs_try_gain_write.3thr \
    pthread_rwlock_fcfs_gain_read.3thr \
    pthread_rwlock_fcfs_gain_write.3thr \
    pthread_rwlock_fcfs_release.3thr \
    pthread_rwlock_fcfs_destroy.3thr

dist_targets += pthread_rwlock_fcfs.html

dist-hook: $(dist_targets)

PODPARAMS=--section=3thr --center="fcfs rwlock" --date="2002/01/02" --release="fcfs rwlock 0.2.0"

pthread_rwlock_fcfs.html: pthread_rwlock_fcfs.pod
	pod2html --noindex --netscape --title="A First-Come First Served Readers/Writers Lock for POSIX Threads" $< > $@

pthread_rwlock_fcfs.3thr: pthread_rwlock_fcfs.pod
	pod2man $(PODPARAMS) $< > $@

EXTRA_DIST += $(man_MANS)
EXTRA_DIST += pthread_rwlock_fcfs.pod create_man_links.sh pthread_rwlock_fcfs.html
