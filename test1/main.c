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



static bool idle_task1( void* arg ){
    static int cnt = 0;

    int i = 0;
    int j = 0;
    const int loop = 100000;
    for( i=0; i<loop; i++){
        for( j=0; j<loop; j++){
        }
    }
    cnt++;
    printf( "idle1 %d i=%d\n", cnt, i );
    if( cnt >= 5 ){
        return false;
    }
    return true;
}


static bool idle_task2( void* arg ){
    static int cnt = 0;

    cnt++;
    printf( "idle2 %d\n", cnt );
    qos_task_sleep_us( 0, 2*US_PER_TICK );

    return true;
}


static bool timer_task( void* arg ){
    static int cnt = 0;

    cnt++;
    printf( "timer %d\n", cnt );
    if( cnt >= 10 ){
        qos_exit();
        return false;
    }
    return true;
}



int main(void){
    qos_init();
    // a timer task called every tick.
    const int task_timer = qos_timer_open( timer_task, NULL, US_PER_TICK );
    qos_task_set_name( task_timer, "task_timer" );
    // a very heavy idle task
    const int task_idle1 = qos_idle_open( idle_task1, NULL );
    qos_task_set_name( task_idle1, "idle1" );
    // a idle task which delay 2 tick.
    const int task_idle2 = qos_idle_open( idle_task2, NULL );
    qos_task_set_name( task_idle2, "idle2" );
    qos_start();
    printf( "qos quit\n" );
    return 0;
}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static bool test_nothing(void){
    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_main(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}

