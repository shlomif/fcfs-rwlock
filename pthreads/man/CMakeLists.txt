CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_VERBOSE_MAKEFILE ON)

INCLUDE( "${CMAKE_CURRENT_SOURCE_DIR}/../Common.cmake" )

RUN_POD2MAN(
    "${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_fcfs.pod"
    "pthread_rwlock_fcfs.3thr"
    "3thr"
    "FCFS RWLock"
    "FCFS RWLock"
)

INSTALL_MAN ( "${CMAKE_CURRENT_BINARY_DIR}/freecell-solver-range-parallel-solve.6" 3)

SET (SRC "${CMAKE_CURRENT_SOURCE_DIR}/pthread_rwlock_fcfs.pod")
SET (DEST "pthread_rwlock_fcfs.html")

ADD_CUSTOM_COMMAND(
    OUTPUT ${DEST}
    COMMAND pod2xhtml
    ARGS "--noindex" "--infile" ${SRC} "--outfile" ${DEST}
    MAIN_DEPENDENCY ${SRC}
    VERBATIM
)

ADD_CUSTOM_TARGET(
    HTML_${DEST} ALL
    DEPENDS ${DEST}
)

SET (man_links "alloc" "timed_gain_read" "timed_gain_write"
    "try_gain_read" "try_gain_write"
    "gain_read" "gain_write"
    "release"
    "destroy")

SET (prefix "pthread_rwlock_fcfs")
FOREACH(link ${man_links})
    SET (full_link "${prefix}_${link}")
    SET (DEST "${full_link}.3thr")
    ADD_CUSTOM_COMMAND(
        OUTPUT ${DEST}
        COMMAND echo
        ARGS ".so man3/${prefix}.3thr" > ${DEST}
        VERBATIM
    )    

    ADD_CUSTOM_TARGET(
        MAN_${DEST} ALL
        DEPENDS ${DEST}
    )   
ENDFOREACH(link)

