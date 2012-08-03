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



static bool qos_task_notify( int task_id, int event, void* arg ){
    uverify( 0 != task_id );
    uverify( 0 != event );
    uverify( NULL == arg );

    if( event == QOS_TASK_EVNET_SLEEP_US ){
        printf( "task_notify: task_id=%d event=%s\n", task_id, "qos_task_sleep_us" );
        return true;
    }

    if( event == QOS_TASK_EVNET_SET_ENTRY ){
        printf( "task_notify: task_id=%d event=%s\n", task_id, "qos_task_set_entry" );
        return true;
    }

    if( event == QOS_TASK_EVNET_CLOSE ){
        printf( "task_notify: task_id=%d event=%s\n", task_id, "qos_task_close" );
        return true;
    }

    if( event == QOS_TASK_EVNET_DELETE ){
        printf( "task_notify: task_id=%d event=%s\n", task_id, "qos_task_delete" );
        return true;
    }

    return true;
}




static bool entry2( void* arg ){
    static int cnt = 0;

    cnt++;
    printf( "entry2 %d\n", cnt );
    qos_task_sleep_us( 0, US_PER_TICK );
    if( 5 < cnt ){
        printf( "entry2 exit\n" );
        qos_exit();
        return false;
    }

    return true;
}
static bool entry1( void* arg ){
    printf( "entry1\n" );
    qos_task_set_entry( 0, entry2, arg );
    return true;
}





int main(void){
    qos_init();
    const int task_id = qos_idle_open( entry1, NULL );
    qos_task_set_notify( task_id, qos_task_notify, NULL );
    qos_start();
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

