/*
 * File      : unit_test_task.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-10     huanglin     first version
 *
 */

/*
unit test by task
    Some function is releated with time, such as 
            int read_something_till_timeout( char* buf );
    How to test it? You cannot test it in a single unit test function.
    But you can test it in a task( timer or idle task ) of qos. This 
file support unit test by task.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inc.h"
#include "qemdlink.h"
#include "qmem.h"


#define this_file_id   file_id_unit_test_task
 

#if GCFG_UNIT_TEST_EN > 0

static bool task_notify_running_test_end( int task_id, int event, void* arg );
static bool start_a_utask(void);


typedef struct{
    ut_task_start_t start;
    qemdlink_node_t node;
}utask_t;




static qemdlink_node_t* utask_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    utask_t* const p = (utask_t*)ptr;
    return &(p->node);
}
static void* utask_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, utask_t, node );
    return p;
}
static int m_utask_link = 0;
static bool utask_link_init(void){
    if( 0 != m_utask_link ){
        return true;
    }

    m_utask_link = qemdlink_open( utask_ptr_to_node, utask_node_to_ptr );
    return_false_if( 0 == m_utask_link );
    return true;
}



// 在链表中，记录启动任务的方法
bool ut_task_open( ut_task_start_t start ){
    uverify( NULL != start );

    return_false_if( !utask_link_init() );

    utask_t* const utask = qmalloc( sizeof( utask_t ) );
    return_false_if( NULL == utask );
    return_false_if( !qemdlink_node_init( &utask->node ) );

    utask->start = start;

    return_false_if( !qemdlink_append_last( m_utask_link, utask ) );
    return true;
}
static ut_task_start_t start_at( int ix ){
    uverify( 0 <= ix );
    return_false_if( !utask_link_init() );

    // 从链表中，取出一个测试项
    utask_t* const utask = qemdlink_at( m_utask_link, ix );
    if( NULL == utask ){
        return NULL;
    }

    const ut_task_start_t start = utask->start;
    uverify( NULL != start );
    return start;
}
// return: first start() form the link. NULL, the link is empty.
static ut_task_start_t first_start_delete_in_link(void){
    return_false_if( !utask_link_init() );

    // 从链表中，取出一个测试项
    utask_t* const utask = qemdlink_first( m_utask_link );
    if( NULL == utask ){
        return NULL;
    }

    const ut_task_start_t start = utask->start;
    return_false_if( !qemdlink_remove( m_utask_link, utask ) );
    if( GCFG_DEBUG_EN ){
        memset( utask, 0, sizeof(utask_t) );
    }
    qfree( utask );
    return start;
}


static bool utask_link_close(void){
    for(;;){
        const ut_task_start_t start = first_start_delete_in_link();
        if( NULL == start ){
            // all node in link delete
            return_false_if( !qemdlink_close( m_utask_link ) );
            m_utask_link = 0;
            return true;
        }
    }

    uverify( false );
    return false;
}











static bool m_running = false;
bool ut_task_running(void){
    return m_running;
}




#define beep( beep_ms ) 
#define sleep_ms( inter_ms )
static void end_bell(void){
    //const int beep_ms = 500;
    //const int inter_ms = 1000;

    beep( beep_ms );
	sleep_ms( inter_ms );
    beep( beep_ms );
	sleep_ms( inter_ms );
    beep( beep_ms );
	sleep_ms( inter_ms );
    beep( beep_ms );
}




static int m_cnt = 0;
static bool ut_task_end(void){
    return_false_if( !utask_link_close() );
    m_running = false;
    end_bell();
    return_false_if( !qos_exit() );
    m_cnt = 0;
    return true;
}
// 所有测试项结束，测试结果ok
static bool ut_task_end_ok(void){
    uverify( unit_test_is_ok() );

    printf( "\nunit test task ok: %d/%d\n", m_cnt+1, GCFG_UNIT_TEST_TIME );

    m_cnt++;
    if( m_cnt < GCFG_UNIT_TEST_TIME ){
        ut_task_pump();
        return true;
    }

    return_false_if( !ut_task_end() );
    return true;
}
// 测试项结束，测试结果失败
static bool ut_task_test_fail(void){
    uverify( !unit_test_is_ok() );
    return_false_if( !ut_task_end() );
    return true;
}




// return: false, stop notify. true, continue notify.
static bool task_notify_running_test_end( int task_id, int event, void* arg ){
    uverify( 0 != task_id );
    uverify( 0 != event );
    uverify( NULL == arg );
    //const int runing_test_task_id = (int)arg;
    //uverify( 0 != runing_test_task_id );

    if( event == QOS_TASK_EVNET_CLOSE ){
        if( !unit_test_is_ok() ){
            // 有任务失败
            return_false_if( !ut_task_test_fail() );
            return false;
        }
        return_false_if( !start_a_utask() );
        return false;
    }

    return true;
}




static bool start_a_utask(void){
    static int ix = 0;
    // 从链表中，取出一个测试项
    const ut_task_start_t start = start_at( ix );
    if( NULL == start ){
        ix = 0;
        return_false_if( !ut_task_end_ok() );
        return true;
    }
    ix++;

    printf( "." );
    // 启动一个测试项
    const int running_test_task_id = start();
    return_false_if( 0 == running_test_task_id );
    return_false_if( !qos_task_set_notify( running_test_task_id, task_notify_running_test_end, NULL ) );
    return true;
}




void ut_task_pump(void){
    printf( "unit test task: %d/%d\n", m_cnt+1, GCFG_UNIT_TEST_TIME );

    if( !start_a_utask() ){
        uverify( false );
    }
    m_running = true;
    return;
}





#endif








//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0

static bool test_item_1(void){
    return true;
}


// unit test entry, add entry in file_list.txt
bool unit_test_unit_test_task(void){
    const unit_test_item_t table[] = {
        test_item_1
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}



// no more------------------------------
