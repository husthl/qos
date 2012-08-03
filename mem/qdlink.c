/*
 * File      : qdlink.c ( qos double link )
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 * double link
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
#include "qdlink.h"

#define this_file_id   file_id_qdlink
 

typedef struct node_s{
    struct node_s* next;
    struct node_s* prev;
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
    const char* const data = qdlink_p( node_id );
    printf( "%d node=0x%08x next=0x%08x data=%c\n", ix, node_id, (int)next, *data );
    return true;
}
static bool qdlink_printf_callback( int node_id, int ix, void* arg ){
    uverify( 0 != node_id );
    qdlink_node_printf_callback_t node_printf = (qdlink_node_printf_callback_t)arg;
    
    if( !node_printf( ix, node_id ) ){
        return false;
    }

    return true;
}
// return: printf node num
int qdlink_printf( int id, qdlink_node_printf_callback_t callback ){
    uverify( id_valid( id ) );

    if( NULL == callback ){
        callback = node_printf_default;
    }

    printf( "\nqdlink<0x%08x>\n", id );
    const int num = qdlink_for_each( id, qdlink_printf_callback, callback );
    printf( "end(total %d)-------\n", num );
    return true;
}





// qdlink id，就是链表的head指针
int qdlink_open(void){
    node_t* const node = malloc( sizeof( node_t ) );
    return_0_if( NULL == node );

    node->next = node;
    node->prev = node;
    return (int)node;
}



bool qdlink_close( int id ){
    uverify( id_valid( id ) );

    qdlink_delete_all( id );
    node_t* const node = (node_t*)id;
    free( node );

    return true;
}





// return: remove node num
int qdlink_delete_all( int id ){
    uverify( id_valid( id ) );

    node_t* const node = (node_t*)id;
    // p pointer to node
    // q pointer to node->next
    node_t* p = NULL;
    node_t* q = NULL;
    int cnt = 0;
    for( p=node->next; p!=node; p=q, cnt++ ){
        q = p->next;
        // 如果在调试状态，将指针清零，便于侦测到错误
        if( GCFG_DEBUG_EN ){
            memset( p, 0, sizeof( node_t ) );
        }
        free( p );
    }

    node->next = node;
    node->prev = node;

    return cnt;
}





//typedef bool (*qdlink_for_eack_t)( int node_id, void* arg );
// return: loop time
int qdlink_for_each( int id, qdlink_for_eack_t callback, void* arg ){
    uverify( id_valid( id ) );

    if( qdlink_empty( id ) ){
        return 0;
    }

    node_t* const node = (node_t*)id;
    // p pointer to node
    node_t* p = NULL;
    int ix = 0;
    for( p=node; p->next!=node; p=p->next, ix++ ){
        const int node_id = (int)(p->next);
        if( !callback( node_id, ix, arg ) ){
            return ix + 1;
        }
    }

    return ix;
}




static bool qdlink_len_callback( int node_id, int ix, void* arg ){
    uverify( node_id_valid( node_id ) );
    uverify( ix_valid( ix ) );
    uverify( NULL == arg );
    return true;
}
int qdlink_len( int id ){
    uverify( id_valid( id ) );

    const int loop_time = qdlink_for_each( id, qdlink_len_callback, NULL );
    return loop_time;
}




bool qdlink_empty( int id ){
    uverify( id_valid( id ) );

    const node_t* const node = (node_t*)id;
    if( node == node->next ){
        uverify( node == node->prev );
        return true;
    }

    return false;
}




// return; node_id
int qdlink_first( int id ){
    uverify( id_valid( id ) );
    
    if( qdlink_empty( id ) ){
        return 0;
    }

    node_t* const node = (node_t*)id;
    node_t* const p = node->next;
    
    return (int)p;
}







// return; node_id
int qdlink_last( int id ){
    uverify( id_valid( id ) );

    if( qdlink_empty( id ) ){
        return 0;
    }

    node_t* const node = (node_t*)id;
    node_t* const p = node->prev;

    return (int)p;
}





// if node_id is the last node, will return 0
// return; node_id
int qdlink_next( int id, int node_id ){
    uverify( id_valid( id ) );
    uverify( node_id_valid( node_id ) );

    node_t* const node = (node_t*)node_id;
    node_t* const p = node->next;
    // last node?
    if( (int)p == id ){
        return 0;
    }
    return (int)p;
}





// if node_id is the first node, will return 0
// return: node_id
int qdlink_prev( int id, int node_id ){
    uverify( id_valid( id ) );
    uverify( node_id_valid( node_id ) );

    node_t* const node = (node_t*)node_id;
    node_t* const p = node->prev;
    // first node?
    if( (int)p == id ){
        return 0;
    }
    return (int)p;
}






typedef struct{
    const int ix;
    int result_id;
}ix_callback_t;
// find: node->next == obj, return node
static bool qdlink_ix_callback( int node_id, int ix, void* void_arg ){
    uverify( node_id_valid( node_id ) );
    ix_callback_t* const arg = (ix_callback_t*)void_arg;
    uverify( ix_valid( arg->ix ) );
    uverify( 0 == arg->result_id );

    if( ix == arg->ix ){
        arg->result_id = node_id;
        return false;
    }

    return true;
}
int qdlink_at( int id, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    ix_callback_t arg = { ix, 0 };
    qdlink_for_each( id, qdlink_ix_callback, &arg );
    return arg.result_id;
}




// node -> next
// =>
// node -> src -> next
// return; ok?
bool qdlink_append( int node_id, int node_src ){
    uverify( node_id_valid( node_id ) );
    uverify( node_id_valid( node_src ) );

    node_t* const node = (node_t*)node_id;
    node_t* const src = (node_t*)node_src;
    uverify( NULL == src->next );
    uverify( NULL == src->prev );

    node_t* const next = node->next;
    node->next = src;
    src->next = next;

    next->prev = src;
    src->prev = node;

    return true;
}




// last->null
// =>
// last->src->null
// return: ok?
bool qdlink_append_last( int id, int node_src ){
    uverify( 0 != id );
    uverify( 0 != node_src );

    int last_id = 0;
    if( qdlink_empty( id ) ){
        last_id = id;
    }
    else{
        last_id = qdlink_last( id );
    }

    uverify( 0 != last_id );
    return_false_if( !qdlink_append( last_id, node_src ) );
    return true;
}




// prev -> node
// =>
// prev -> src -> node
// return: ok?
bool qdlink_insert( int id, int node_id, int node_src ){
    uverify( id_valid( id ) );
    uverify( node_id_valid( node_id ) );
    uverify( node_id_valid( node_src ) );

    int prev_id = 0;
    if( node_id == qdlink_first( id ) ){
        prev_id = id;
    }
    else{
        prev_id = qdlink_prev( id, node_id );
        // not found the node
        uverify( 0 != prev_id );
    }

    uverify( 0 != prev_id );
    return_false_if( !qdlink_append( prev_id, node_src ) );

    return true;
}




// node_t + data_room
// return: node id
int qdlink_malloc( int size ){
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
    node->prev = NULL;
    return (int)p;
}




// node id to data pointer
// return: data pointer
void* qdlink_p( int node_id ){
    uverify( node_id_valid( node_id ) );

    char* const p = (char*)node_id;
    char* data_p = p + sizeof( node_t );
    return data_p;
}





// prev -> node -> next
// =>
// prev -> next
// return: ok?
bool qdlink_delete( int id, int node_id ){
    uverify( id_valid( id ) );
    uverify( node_id_valid( node_id ) );

    int prev_id = 0;
    if( node_id == qdlink_first( id ) ){
        prev_id = id;
    }
    else{
        prev_id = qdlink_prev( id, node_id );
        // not found the node
        uverify( 0 != prev_id );
    }

    return_false_if( 0 == prev_id );

    node_t* const prev = (node_t*)prev_id;
    node_t* const node = (node_t*)node_id;
    node_t* const next = node->next;

    prev->next = next;
    next->prev = prev;

    if( GCFG_DEBUG_EN ){
        node->next = NULL;
        node->prev = NULL;
    }
    free( node );
    return true;
}







// 单元测试例子及模板
#if GCFG_UNIT_TEST_EN > 0



static bool test_open_close(void){
    int id = qdlink_open();
    test( qdlink_close( id ) );
    id = 0;
    return true;
}



static bool test_1_node(void){
    // head
    int id = qdlink_open();

    {
        test( qdlink_empty( id ) );
        test( 0 == qdlink_len( id ) );

        test( 0 == qdlink_first( id ) );
        test( 0 == qdlink_last( id ) );
        test( 0 == qdlink_at( id, 0 ) );
    }

    {
        const int n0 = qdlink_malloc( sizeof( char ) );
        char* const p = qdlink_p( n0 );
        *p = '0';
        test( qdlink_append_last( id, n0 ) );
        // head->n0

        test( !qdlink_empty( id ) );
        test( 1 == qdlink_len( id ) );

        const int first = qdlink_first( id );
        test( n0 == first );
        const int last = qdlink_last( id );
        test( n0 == last );
        test( '0' == (*(char*)qdlink_p( first )) );

        test( 0 == qdlink_next( id, first ) );
        test( 0 == qdlink_prev( id, first ) );
        
        test( first == qdlink_at( id, 0 ) );
        test( 0 == qdlink_at( id, 1 ) );

    }

    {
        const int n0 = qdlink_first( id );
        test( qdlink_delete( id, n0 ) );
        // head

        test( qdlink_empty( id ) );
        test( 0 == qdlink_len( id ) );
    }

    {
        const int n1 = qdlink_malloc( sizeof( char ) );
        char* const p = qdlink_p( n1 );
        *p = '1';
        test( qdlink_append_last( id, n1 ) );
        // head->n1

        test( !qdlink_empty( id ) );
        test( 1 == qdlink_len( id ) );
    }

    test( qdlink_close( id ) );
    id = 0;
    return true;
}



static bool test_2_node(void){
    int id = qdlink_open();
    // head

    // append last
    {
        const int n0 = qdlink_malloc( sizeof( char ) );
        char* const p = qdlink_p( n0 );
        *p = '0';
        test( qdlink_append_last( id, n0 ) );
        // head->n0
    }
    {
        const int n1 = qdlink_malloc( sizeof( char ) );
        char* const p = qdlink_p( n1 );
        *p = '1';
        test( qdlink_append_last( id, n1 ) );
        // head->n0->n1
    }
    {
        test( !qdlink_empty( id ) );
        test( 2 == qdlink_len( id ) );

        const int first = qdlink_first( id );
        test( 0 != first );
        test( '0' == (*(char*)qdlink_p( first )) );
        const int last = qdlink_last( id );
        test( 0 != last );
        test( first != last );
        test( '1' == (*(char*)qdlink_p( last )) );

        test( last == qdlink_next( id, first ) );
        test( first == qdlink_prev( id, last ) );
        
        test( first == qdlink_at( id, 0 ) );
        test( last == qdlink_at( id, 1 ) );
        test( 0 == qdlink_at( id, 2 ) );
    }

    {
        const int n0 = qdlink_first( id );
        test( '0' == (*(char*)qdlink_p( n0 )) );
        test( qdlink_delete( id, n0 ) );
        // head->n1

        test( !qdlink_empty( id ) );
        test( 1 == qdlink_len( id ) );
    }

    // insert
    {
        const int n1 = qdlink_last( id );
        test( '1' == (*(char*)qdlink_p( n1 )) );

        const int n0 = qdlink_malloc( sizeof( char ) );
        char* const p = qdlink_p( n0 );
        *p = '0';
        test( qdlink_insert( id, n1, n0 ) );
        // head->n0->n1

        test( !qdlink_empty( id ) );
        test( 2 == qdlink_len( id ) );
    }

    {
        const int n1 = qdlink_last( id );
        test( '1' == (*(char*)qdlink_p( n1 )) );
        test( qdlink_delete( id, n1 ) );
        // head->n0
    }

    // append
    {
        const int n1 = qdlink_malloc( sizeof( char ) );
        char* const p = qdlink_p( n1 );
        *p = '1';

        const int n0 = qdlink_at( id, 0 );
        test( '0' == (*(char*)qdlink_p( n0 )) );
        test( qdlink_append( n0, n1 ) );
        // head->n0->n1
    }

    // remove all
    {
        test( 2 == qdlink_delete_all( id ) );
        test( qdlink_empty( id ) );
        test( 0 == qdlink_len( id ) );
    }

    test( qdlink_close( id ) );
    id = 0;

    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_qdlink(void){
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
