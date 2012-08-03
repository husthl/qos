/*
 * File      : qmem_task.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-11     huanglin     first version
 *
 */


#include <stdio.h>

#include "inc.h"
#include "qmem.h"

#define this_file_id   file_id_qmem_task
 

#define QMEM_CHECK_INTERVAL_SEC         (30)


static bool qmem_check_fail_printf( void* ptr ){
    uverify( NULL != ptr );

    mem_info_t mem_info;
    return_false_if( !qmem_info( &mem_info, ptr ) );

    qmem_edge_t edge;
    return_false_if( !qmem_edge( &edge, ptr ) );

    char const* file_name = file_id_to_str( mem_info.file_id );
    printf( "mem check FAIL: malloc in %d@%s, ptr=0x%08x\n", mem_info.line, file_name, (int)ptr );
    printf( "left edge,  real:%.*s(hex: %02x%02x%02x%02x) expect:%.*s\n"
            , edge.left_len, edge.left
            , edge.left[0], edge.left[1], edge.left[2], edge.left[3]
            , edge.left_len, edge.left_pattern );
    printf( "right edge, real:%.*s(hex: %02x%02x%02x%02x) expect:%.*s\n"
            , edge.right_len, edge.right
            , edge.right[0], edge.right[1], edge.right[2], edge.right[3]
            , edge.right_len, edge.right_pattern );
    printf( "\n" );
    return true;
}



static bool check_all_callback( const mem_info_t* mem_info, int ix, void* arg ){
    uverify( NULL != mem_info );
    uverify( 0 <= ix );
    uverify( NULL != arg );

    void* const ptr = mem_info->ptr;
    if( !qmem_check( ptr ) ){
        bool* const ok = arg;
        *ok = false;

        return_false_if( !qmem_check_fail_printf( ptr ) );
        // check all memory
    }
    return true;
}
// return: ok? if check fail, mem store the 1st error mem infomation.
static bool check_all(void){
    bool ok = true;
    qmem_for_each( check_all_callback, &ok );
    return ok;
}




// edge check
// if check any memory fail, stop the task. Decrease the screen output.
static bool qmem_task_entry( void* arg ){
    if( !check_all() ){
        return false;
    }
    return true;
}



static bool printf_leak_memory_callback( const mem_info_t* mem_info, int ix, void* arg ){
    uverify( NULL != mem_info );
    uverify( 0 <= ix );
    uverify( NULL == arg );

    char const* file_name = file_id_to_str( mem_info->file_id );
    const void* const ptr = mem_info->ptr;
    printf( "%d malloc in %d@%s, ptr=0x%08x\n", ix, mem_info->line, file_name, (int)ptr );
    return true;
}
// return: ok? if check fail, mem store the 1st error mem infomation.
static bool printf_leak_memory(void){
    if( 0 != qmem_count() ){
        printf( "\n\nmemory leak:\n" );
    }
    qmem_for_each( printf_leak_memory_callback, NULL );
    return true;
}




// check memory leak
bool qmem_task_at_exit(void){
    if( 0 == GCFG_MEM_EN ){
        return true;
    }

    return_false_if( !printf_leak_memory() );
    return_false_if( !check_all() );
    return true;
}



bool qmem_task_init(void){
    if( 0 == GCFG_MEM_EN ){
        return true;
    }

    const int interval_us = SEC_TO_US( QMEM_CHECK_INTERVAL_SEC );
    const int task_id = qos_timer_open( qmem_task_entry, NULL, interval_us );
    return_false_if( 0 == task_id );
    return_false_if( !qos_task_set_name( task_id, "qmem_task_entry" ) );
    return true;
}


//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0

static bool test_nothing(void){
    return true;
}



// unit test entry, add entry in file_list.h
bool unit_test_qmem_task(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}




// no more------------------------------
