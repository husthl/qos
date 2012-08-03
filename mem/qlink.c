/*
 * File      : qlink.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-06-29     huanglin     first version
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inc.h"
#include "qlink.h"

#define this_file_id   file_id_qlink
 

typedef struct node_s{
    struct node_s* next;
}node_t;


static bool id_valid( int id ){
    if( 0 == id ){
        return false;
    }

    return true;
}


static bool node_id_valid( int node_id ){
    if( 0 == node_id ){
        return false;
    }

    return true;
}



static bool ix_valid( int ix ){
    if( 0 > ix ){
        return false;
    }

    return true;
}




static bool node_printf_default( int ix, int node_id ){
    if( 0 == node_id ){
        printf( "%d <NULL>", ix );
        return true;
    }

    const node_t* const node = (node_t*)node_id;
    const node_t* const next = node->next;
    const char* const data = qlink_p( node_id );
    printf( "%d node=0x%08x next=0x%08x data=%c\n", ix, node_id, (int)next, *data );
    return true;
}
static bool qlink_printf_callback( int node_id, int ix, void* arg ){
    uverify( 0 != node_id );
    qlink_node_printf_callback_t node_printf = (qlink_node_printf_callback_t)arg;
    
    if( !node_printf( ix, node_id ) ){
        return false;
    }

    return true;
}
// return: printf node num
int qlink_printf( int id, qlink_node_printf_callback_t callback ){
    uverify( 0 != id );
    if( NULL == callback ){
        callback = node_printf_default;
    }

    printf( "\nqlink<0x%08x>\n", id );
    const int num = qlink_for_each( id, qlink_printf_callback, callback );
    printf( "end(total %d)-------\n", num );
    return true;
}





// qlink id，就是链表的head指针
int qlink_open(void){
    node_t* const node = malloc( sizeof( node_t ) );
    return_0_if( NULL == node );

    node->next = NULL;
    return (int)node;
}



bool qlink_close( int id ){
    uverify( id_valid( id ) );

    qlink_delete_all( id );
    node_t* const node = (node_t*)id;
    free( node );

    return true;
}


// return: the remove nodes count
int qlink_delete_all( int id ){
    uverify( 0 != id );

    node_t* const node = (node_t*)id;
    // p pointer to node
    // q pointer to node->next
    node_t* p = NULL;
    node_t* q = NULL;
    int cnt = 0;
    for( p=node->next; p!=NULL; p=q, cnt++ ){
        q = p->next;
        // 如果在调试状态，将指针清零，便于侦测到错误
        if( GCFG_DEBUG_EN ){
            memset( p, 0, sizeof( node_t ) );
        }
        free( p );
    }

    return cnt;
}


//typedef bool (*qlink_for_eack_t)( int node_id, void* arg );
// return: loop time
int qlink_for_each( int id, qlink_for_eack_t callback, void* arg ){
    uverify( 0 != id );

    if( qlink_empty( id ) ){
        return 0;
    }

    node_t* const node = (node_t*)id;
    // p pointer to node
    node_t* p = NULL;
    int ix = 0;
    for( p=node; p->next!=NULL; p=p->next, ix++ ){
        const int node_id = (int)(p->next);
        if( !callback( node_id, ix, arg ) ){
            return ix + 1;
        }
    }

    return ix;
}






static bool qlink_len_callback( int node_id, int ix, void* arg ){
    uverify( 0 != node_id );
    uverify( NULL == arg );
    return true;
}
int qlink_len( int id ){
    uverify( 0 != id );

    const int loop_time = qlink_for_each( id, qlink_len_callback, NULL );
    return loop_time;
}




bool qlink_empty( int id ){
    uverify( 0 != id );

    const node_t* const node = (node_t*)id;
    if( NULL == node->next ){
        return true;
    }

    return false;
}




// return; node_id
int qlink_first( int id ){
    uverify( 0 != id );
    
    node_t* const node = (node_t*)id;
    node_t* const p = node->next;
    
    return (int)p;
}







static bool qlink_last_callback( int node_id, int ix, void* arg ){
    uverify( node_id_valid( node_id ) );
    uverify( ix_valid( ix ) );
    int* const last_node_id = (int*)arg;
    uverify( NULL != last_node_id );
    uverify( 0 == *last_node_id );

    node_t* const node = (node_t*)node_id;
    if( NULL == node->next ){
        *last_node_id = node_id;
        return false;
    }
    return true;
}
// return; node_id
int qlink_last( int id ){
    uverify( 0 != id );

    if( qlink_empty( id ) ){
        return 0;
    }

    int last_node_id = 0;
    qlink_for_each( id, qlink_last_callback, &last_node_id );
    return last_node_id;
}





// return; node_id
int qlink_next( int node_id ){
    uverify( 0 != node_id );

    node_t* const node = (node_t*)node_id;
    return (int)(node->next);
}





typedef struct{
    int obj_id;
    int prev_id;
}prev_callback_t;
// find: node->next == obj, return node
static bool qlink_prev_callback( int node_id, int ix, void* void_arg ){
    uverify( 0 != node_id );
    prev_callback_t* const arg = (prev_callback_t*)void_arg;
    uverify( 0 != arg->obj_id );
    uverify( 0 == arg->prev_id );

    const node_t* const node = (node_t*)node_id;
    if( arg->obj_id == (int)(node->next) ){
        arg->prev_id = node_id;
        return false;
    }
    return true;
}
// return: if node_id is the first node, will return 0
int qlink_prev( int id, int node_id ){
    uverify( 0 != id );
    uverify( 0 != node_id );

    prev_callback_t arg = { node_id, 0 };
    qlink_for_each( id, qlink_prev_callback, &arg );
    return arg.prev_id;
}






typedef struct{
    const int ix;
    int result_id;
}ix_callback_t;
// find: node->next == obj, return node
static bool qlink_ix_callback( int node_id, int ix, void* void_arg ){
    uverify( 0 != node_id );
    ix_callback_t* const arg = (ix_callback_t*)void_arg;
    uverify( 0 <= arg->ix );
    uverify( 0 == arg->result_id );

    if( ix == arg->ix ){
        arg->result_id = node_id;
        return false;
    }

    return true;
}
int qlink_at( int id, int ix ){
    uverify( 0 != id );
    uverify( 0 <= ix );

    ix_callback_t arg = { ix, 0 };
    qlink_for_each( id, qlink_ix_callback, &arg );
    return arg.result_id;
}




// node -> next
// =>
// node -> src -> next
// return; ok?
bool qlink_append( int node_id, int node_src ){
    uverify( 0 != node_id );
    uverify( 0 != node_src );

    node_t* const node = (node_t*)node_id;
    node_t* const src = (node_t*)node_src;
    uverify( NULL == src->next );

    node_t* const next = node->next;
    node->next = src;
    src->next = next;

    return true;
}




// last->null
// =>
// last->src->null
// return: ok?
bool qlink_append_last( int id, int node_src ){
    uverify( 0 != id );
    uverify( 0 != node_src );

    int last_id = 0;
    if( qlink_empty( id ) ){
        last_id = id;
    }
    else{
        last_id = qlink_last( id );
    }

    uverify( 0 != last_id );
    return_false_if( !qlink_append( last_id, node_src ) );
    /*
    node_t* const last = (node_t*)last_id;
    node_t* const src = (node_t*)node_src;
    uverify( NULL == src->next );

    last->next = src;
    */
    return true;
}




// prev -> node
// =>
// prev -> src -> node
// return: ok?
bool qlink_insert( int id, int node_id, int node_src ){
    uverify( 0 != id );
    uverify( 0 != node_id );
    uverify( 0 != node_src );

    int prev_id = 0;
    if( node_id == qlink_first( id ) ){
        prev_id = id;
    }
    else{
        prev_id = qlink_prev( id, node_id );
        // not found the node
        if( 0 == prev_id ){
            return false;
        }
    }

    uverify( 0 != prev_id );
    return_false_if( !qlink_append( prev_id, node_src ) );
    /*
    node_t* const prev = (node_t*)prev_id;
    node_t* const node = (node_t*)node_id;
    node_t* const src = (node_t*)node_src;
    uverify( NULL == src->next );

    prev->next = src;
    src->next = node;
    */

    return true;
}




// node_t + data_room
// return: node id
int qlink_malloc( int size ){
    uverify( 0 <= size );
    // 避免内存对齐，引起data空间分配不足
    uverify( 0 == sizeof( node_t ) % 4 );
    const int real_size = sizeof( node_t ) + size;
    void* const p = malloc( real_size );

    // 为侦测出更多的错误，调试状态，将数据区清零
    if( GCFG_DEBUG_EN ){
        char* const data_p = p + sizeof( node_t );
        memset( data_p, 0, size );
    }

    node_t* const node = (node_t*)p;
    node->next = NULL;
    return (int)p;
}




// node id to data pointer
// return: data pointer
void* qlink_p( int node_id ){
    uverify( node_id_valid( node_id ) );
    char* const p = (char*)node_id;
    char* data_p = p + sizeof( node_t );
    return data_p;
}





// prev -> node -> next
// =>
// prev -> next
// return: ok?
bool qlink_delete( int id, int node_id ){
    uverify( id_valid( id ) );
    uverify( 0 != node_id );

    int prev_id = 0;
    if( node_id == qlink_first( id ) ){
        prev_id = id;
    }
    else{
        prev_id = qlink_prev( id, node_id );
        // not found the node
        if( 0 == prev_id ){
            return false;
        }
    }

    return_false_if( 0 == prev_id );
    node_t* const prev = (node_t*)prev_id;
    node_t* const node = (node_t*)node_id;
    node_t* const next = node->next;

    prev->next = next;

    if( GCFG_DEBUG_EN ){
        node->next = NULL;
    }
    free( node );
    return true;
}







// 单元测试例子及模板
#if GCFG_UNIT_TEST_EN > 0






static bool test_open_close(void){
    int id = qlink_open();
    test( qlink_close( id ) );
    id = 0;
    return true;
}



static bool test_1_node(void){
    // head
    int id = qlink_open();

    {
        test( qlink_empty( id ) );
        test( 0 == qlink_len( id ) );

        const int first = qlink_first( id );
        test( 0 == first );
        const int last = qlink_last( id );
        test( 0 == last );

    }

    // head->n0
    {
        const int n0 = qlink_malloc( sizeof( char ) );
        test( qlink_append_last( id, n0 ) );

        char* const p = qlink_p( n0 );
        *p = '0';
    }

    {
        test( !qlink_empty( id ) );
        test( 1 == qlink_len( id ) );

        const int first = qlink_first( id );
        test( 0 != first );
        test( '0' == (*(char*)qlink_p( first )) );
        const int last = qlink_last( id );
        test( 0 != last );
        test( first == last );
        test( '0' == (*(char*)qlink_p( last )) );

        test( 0 ==  qlink_next( first ) );
        test( 0 == qlink_prev( id, first ) );
        
        test( first == qlink_at( id, 0 ) );
        test( 0 == qlink_at( id, 1 ) );

    }

    // head
    {
        const int n0 = qlink_first( id );
        test( qlink_delete( id, n0 ) );

        test( qlink_empty( id ) );
        test( 0 == qlink_len( id ) );
    }

    // head->n1
    {
        const int n1 = qlink_malloc( sizeof( char ) );
        char* const p = qlink_p( n1 );
        *p = '1';
        test( qlink_append_last( id, n1 ) );

        test( !qlink_empty( id ) );
        test( 1 == qlink_len( id ) );
    }

    test( qlink_close( id ) );
    id = 0;
    return true;
}



static bool test_2_node(void){
    // head
    int id = qlink_open();

    // head->n0->n1
    {
        const int n0 = qlink_malloc( sizeof( char ) );
        char* const p = qlink_p( n0 );
        *p = '0';
        test( qlink_append_last( id, n0 ) );

    }
    {
        const int n1 = qlink_malloc( sizeof( char ) );
        char* const p = qlink_p( n1 );
        *p = '1';
        test( qlink_append_last( id, n1 ) );
    }

    {
        test( !qlink_empty( id ) );
        test( 2 == qlink_len( id ) );

        const int first = qlink_first( id );
        test( 0 != first );
        test( '0' == (*(char*)qlink_p( first )) );
        const int last = qlink_last( id );
        test( 0 != last );
        test( first != last );
        test( '1' == (*(char*)qlink_p( last )) );

        test( last == qlink_next( first ) );
        test( first == qlink_prev( id, last ) );
        
        test( first == qlink_at( id, 0 ) );
        test( last == qlink_at( id, 1 ) );
        test( 0 == qlink_at( id, 2 ) );
    }

    // head->n1
    {
        const int n0 = qlink_first( id );
        test( '0' == (*(char*)qlink_p( n0 )) );
        test( qlink_delete( id, n0 ) );

        test( !qlink_empty( id ) );
        test( 1 == qlink_len( id ) );
    }

    // head->n0->n1
    {
        const int n1 = qlink_last( id );
        test( '1' == (*(char*)qlink_p( n1 )) );

        const int n0 = qlink_malloc( sizeof( char ) );
        char* const p = qlink_p( n0 );
        *p = '0';
        test( qlink_insert( id, n1, n0 ) );

        test( !qlink_empty( id ) );
        test( 2 == qlink_len( id ) );
    }

    test( qlink_close( id ) );
    id = 0;

    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_qlink(void){
    const unit_test_item_t table[] = {
        test_open_close
        , test_1_node
        , test_2_node
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif



// no more------------------------------
