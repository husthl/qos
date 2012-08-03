/*
 * File      : main.c
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
#include "inc.h"


#define this_file_id   file_id_main




static bool entry( void* arg ){
    static int cnt = 0;

    cnt++;
    printf( "entry %d\n", cnt );
    qos_task_sleep_us( 0, US_PER_TICK );
    if( 5 < cnt ){
        printf( "entry exit\n" );
        qos_exit();
        return false;
    }

    return true;
}


static bool app_init(void){
    if( GCFG_UNIT_TEST_EN ){
        return true;
    }
    qos_idle_open( entry, NULL );
    return true;
}
int main(void){
    qos_init();
    app_init();
    qos_start();
    return 0;
}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0



static bool test_entry1( void* arg ){
    static int cnt = 0;

    cnt++;
    if( 3 == cnt ){
        test( 1 == 1 );
    }

    //printf( "test_entry1 %d\n", cnt );
    if( 5 < cnt ){
        //printf( "test_entry1 exit\n" );
        return false;
    }
    return true;
}
static int ut_task_start1(void){
    return qos_idle_open( test_entry1, NULL );
}
static bool test_in_idle(void){
    test( ut_task_open( ut_task_start1 ) );
    return true;
}



static bool test_entry2( void* arg ){
    static int cnt = 0;

    cnt++;
    if( 3 == cnt ){
        test( 2 == 2 );
    }

    //printf( "test_entry2 %d\n", cnt );
    if( 5 < cnt ){
        //printf( "test_entry2 exit\n" );
        return false;
    }
    return true;
}
static int ut_task_start2(void){
    return qos_timer_open( test_entry2, NULL, 1*US_PER_TICK );
}
static bool test_in_timer(void){
    test( ut_task_open( ut_task_start2 ) );
    return true;
}

// unit test entry, add entry in file_list.h
bool unit_test_main(void){
    const unit_test_item_t table[] = {
        test_in_idle
        , test_in_timer
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}

