#!/bin/bash
PREFIX=pthread_rwlock_fcfs
for I in alloc {timed_,try_,}gain_{'read',write} release destroy ; do
    echo ".so man3/${PREFIX}.3thr" > ${PREFIX}_$I.3thr
    echo "    ${PREFIX}_$I.3thr \\"
done

