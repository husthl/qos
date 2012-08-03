/*
 * File      : qos_linux.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-06     huanglin     first version
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "errno.h"

#include "inc.h"
#include "qmem.h"

#define this_file_id   file_id_qos_linux


static void sig_tick( int signo );


//{{{ elapse


/*
static bool linux_sleep_us( int us ){
    struct timespec req;

    req.tv_sec = us / 1000000;
    req.tv_nsec = us % 1000000;
    const int rc = nanosleep( &req, NULL );
    if( 0 != rc ){
        printf( "rc=%d errno=%d\n", rc, errno );
        perror( "" );
        return false;
    }
    return true;
}
*/

#if 0
// usage: 
// struct timeval start;
// linux_elpase_start( &start );
// const int elapse_us = linux_elapse_end_us( &start );
static bool linux_elpase_start( struct timeval* start ){
    return_false_if( 0 != gettimeofday( start, NULL ) );
    return true;
}
// return: elapse us. If error, return < 0.
static int linux_elapse_end_us( struct timeval* start ){
    uverify( NULL != start );

    struct timeval end;
    if( 0 != gettimeofday( &end, NULL ) ){
        uverify( false );
        return -1;
    }
    const int us_per_sec = ( MS_TO_US( SEC_TO_MS( 1 ) ) );
    const int elapse_us = us_per_sec * ( end.tv_sec - start->tv_sec ) + end.tv_usec - start->tv_usec; 
    /*
    if( 0 > elapse_us ){
        var( us_per_sec );
        var( end.tv_sec );
        var( end.tv_usec );
        var( start->tv_sec );
        var( start->tv_usec );
        var( elapse_us );
    }
    */
    uverify( 0 <= elapse_us );
    return elapse_us;
}


// return: elpase_id
int elapse_us_open(void){
    struct timeval* const start = qmalloc( sizeof( struct timeval ) );
    return_0_if( NULL == start );

    return_0_if( !linux_elpase_start( start ) );
    const int elapse_id = (int)start;
    return elapse_id;
}
// return: elapse us. if error, return <0;
int elapse_us_reset( int elapse_id ){
    uverify( 0 != elapse_id );
    const int pass_us = elapse_us_update( elapse_id );

    struct timeval* const start = (struct timeval*)elapse_id;
    if( !linux_elpase_start( start ) ){
        uverify( false );
        return -1;
    }
    return pass_us;
}
// return: elapse us
int elapse_us_update( int elapse_id ){
    uverify( 0 != elapse_id );
    struct timeval* const start = (struct timeval*)elapse_id;
    const int pass_us = linux_elapse_end_us( start );
    uverify( 0 <= pass_us );
    return pass_us;
}
// return: elapse us. If error, return < 0.
int elapse_us_close( int elapse_id ){
    uverify( 0 != elapse_id );
    const int pass_us = elapse_us_update( elapse_id );

    struct timeval* const start = (struct timeval*)elapse_id;
    qfree( start );
    return pass_us;
}

#endif



// usage: 
// struct timespec start;
// linux_elpase_start( &start );
// const int elapse_us = linux_elapse_end_us( &start );
static bool linux_elpase_start( struct timespec* start ){
    uverify( NULL != start );
    return_false_if( 0 != clock_gettime( CLOCK_REALTIME, start ) );
    return true;
}
// return: elapse us. If error, return < 0.
static int linux_elapse_end_us( struct timespec* start ){
    uverify( NULL != start );

    struct timespec end;
    if( 0 != clock_gettime( CLOCK_REALTIME, &end ) ){
        uverify( false );
        return -1;
    }
    const int us_per_sec = ( MS_TO_US( SEC_TO_MS( 1 ) ) );
    const int volatile elapse_us = us_per_sec * ( end.tv_sec - start->tv_sec ) + ( end.tv_nsec - start->tv_nsec )/1000; 
    if( 0 > elapse_us ){
        var( us_per_sec );
        var( end.tv_sec );
        var( end.tv_nsec );
        var( start->tv_sec );
        var( start->tv_nsec );
        var( elapse_us );
    }
    uverify( 0 <= elapse_us );
    return elapse_us;
}



// return: elpase_id
int elapse_us_open(void){
    struct timespec* const start = qmalloc( sizeof( struct timespec ) );
    return_0_if( NULL == start );

    return_0_if( !linux_elpase_start( start ) );
    const int elapse_id = (int)start;
    return elapse_id;
}
// return: elapse us. if error, return <0;
int elapse_us_reset( int elapse_id ){
    uverify( 0 != elapse_id );
    const int pass_us = elapse_us_update( elapse_id );

    struct timespec* const start = (struct timespec*)elapse_id;
    if( !linux_elpase_start( start ) ){
        uverify( false );
        return -1;
    }
    return pass_us;
}
// return: elapse us
int elapse_us_update( int elapse_id ){
    uverify( 0 != elapse_id );
    struct timespec* const start = (struct timespec*)elapse_id;
    const int pass_us = linux_elapse_end_us( start );
    uverify( 0 <= pass_us );
    return pass_us;
}
// return: elapse us. If error, return < 0.
int elapse_us_close( int elapse_id ){
    uverify( 0 != elapse_id );
    const int pass_us = elapse_us_update( elapse_id );

    struct timespec* const start = (struct timespec*)elapse_id;
    qfree( start );
    return pass_us;
}






//}}}

static bool linux_init_sigaction(void){
    struct sigaction act;
    const int signo = SIGALRM;

    act.sa_handler = sig_tick;
    act.sa_flags   = 0;
    sigemptyset( &act.sa_mask );

    return_false_if( 0 != sigaction( signo, &act, NULL ) );
    return true;
}
static bool linux_stop_sigaction(void){
    struct sigaction act;
    const int signo = SIGALRM;

    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    return_false_if( 0 != sigaction( signo, &act, NULL ) );
    return true;
}


#define US_PER_SEC     ( MS_TO_US( SEC_TO_MS( 1 ) ) )
static bool linux_init_time(void){
    struct itimerval val;

    int sec = 0;
    int us = 0;
    // if US_PER_TICK > 1000000, setitimer() return EINVAL
    if( US_PER_TICK >= US_PER_TICK ){
        sec = US_PER_TICK / US_PER_SEC;
        us = US_PER_TICK - sec * US_PER_SEC;
    }
    else{
        sec = 0;
        us = US_PER_TICK;
    }

    val.it_value.tv_sec = sec;
    val.it_value.tv_usec = us;
    val.it_interval = val.it_value;
    return_false_if( 0 != setitimer( ITIMER_REAL, &val, NULL ) );
    return true;
}
static bool linux_stop_time(void){
    struct itimerval val;

    val.it_value.tv_sec = 0;
    val.it_value.tv_usec = 0;
    val.it_interval = val.it_value;
    return_false_if( 0 != setitimer( ITIMER_REAL, &val, NULL ) );
    return true;
}







/*
struct timeval m_tick_start;
static bool in_idle( struct timeval* tick_start ){
    uverify( NULL != tick_start );

    const int tick_pass_us = linux_elapse_end_us( tick_start );
    if( US_PER_TICK <= tick_pass_us ){
        return false;
    }

    const int idle_us = US_PER_TICK - tick_pass_us;
    // 为了运行idle，需要的最少空闲时�?
    const int idle_us_min = GCFG_QOS_IDLE_US_MIN;
    //printf( "idle_us=%d\n", idle_us );
    if( idle_us_min > idle_us ){
        return false;
    }

    return true;
}
bool qos_in_idle(void){
    return in_idle( &m_tick_start );
}
*/



static volatile bool m_wait_signal = true;
static bool stop_tick(void){
    if( !linux_stop_time() ){
        uverify( false );
    }
    if( !linux_stop_sigaction() ){
        uverify( false );
    }
    m_wait_signal = false;
    //exit( 0 );
    return true;
}






static void sig_tick( int signo ){
    uverify( SIGALRM == signo );

    /*
    if( !linux_elpase_start( &m_tick_start ) ){
        uverify( false );
        stop_tick();
        return;
    }
    */

    if( !qos_tick() ){
        stop_tick();
        return;
    }

    // tick again
    return;
}



bool qos_start(void){
    return_false_if( !linux_init_sigaction() );
    return_false_if( !linux_init_time() );

    while( m_wait_signal ) {  
        pause();  
    }  
    return true;
}






//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static bool test_nothing(void){
    return true;
}



// unit test entry, add entry in file_list.h
bool unit_test_qos_linux(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}




// no more------------------------------
