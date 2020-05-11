#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cctk.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

#include <pthread.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static time_t timestamp;
static struct {
    int timeout_sec;
    int mpi_rank;
} param;

static void * patrol(void * arg) {
    time_t time_old, time_new, ltime;
    char tstamp[128];

    pthread_mutex_lock(&mutex);
    time_old = timestamp;
    pthread_mutex_unlock(&mutex);

    if(0 == param.mpi_rank) {
        ltime = time(NULL);
        asctime_r(localtime(&ltime), &tstamp[0]);
        tstamp[24] = '\0';
        fprintf(stderr, "[WATCHDOG (%s)] Starting.\n", tstamp);
        fflush(stderr);
    }

    while(true) {
        unsigned int left = param.timeout_sec;
        while(left > 0) {
            left = sleep(left);
        }

        pthread_mutex_lock(&mutex);
        time_new = timestamp;
        pthread_mutex_unlock(&mutex);

        ltime = time(NULL);
        asctime_r(localtime(&ltime), &tstamp[0]);
        tstamp[24] = '\0';
        if(time_new == time_old) {
            fprintf(stderr, "[WATCHDOG (%s)] Rank %d is not progressing.\n",
                    tstamp, param.mpi_rank);
            fprintf(stderr, "[WATCHDOG (%s)] Terminating...\n", tstamp);
            fflush(stderr);
            abort();
        }
        else {
            if(0 == param.mpi_rank) {
                fprintf(stderr, "[WATCHDOG (%s)] Everything is fine.\n",
                        tstamp);
                fflush(stderr);
            }
            time_old = time_new;
        }
    }
}

void WatchDog(CCTK_ARGUMENTS) {
    DECLARE_CCTK_ARGUMENTS_WatchDog
    DECLARE_CCTK_PARAMETERS

    pthread_mutex_lock(&mutex);
    timestamp = time(NULL);
    pthread_mutex_unlock(&mutex);

    static bool first_time = true;
    if(first_time) {
        param.timeout_sec = check_every;
        param.mpi_rank = CCTK_MyProc(cctkGH);
        pthread_t dog; /* not used beyond passed to phread_ceate */
        int ierr = pthread_create(&dog, NULL, patrol, NULL);
        if(ierr) {
            CCTK_ERROR("Dispatching watch dog thread failed");
        }
        first_time = false;
    }
}
