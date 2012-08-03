/*
 * File      : qemdlink.c ( qos embeded double link )
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
#include "qemdlink.h"
#include "qmem.h"

#define this_file_id   file_id_qemdlink


typedef qemdlink_node_t node_t;
 


static bool id_valid( int id ){
    if( 0 == id ){
        return false;
    }

    return true;
}
bool qemdlink_id_valid( int id ){
    return id_valid( id );
}


static bool ptr_valid( const void* ptr ){
    if( NULL == ptr ){
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


#if 1
static bool qemdlink_printf_callback( int id, void* ptr, int ix, void* arg ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr ) );
    uverify( ix_valid( ix ) );
    const int ptr_size = (int)arg;
    uverify( 0 < ptr_size );
    
    printf( "%d. ptr=0x%08x ", ix, (int)ptr );
    const char* p = (char*)ptr;
    int i = 0;
    for( i=0; i<ptr_size; i++ ){
        printf( "%x ", p[i] );
    }
    printf( "\n" );
    return true;
}
// return: printf node num
int qemdlink_printf( int id, int ptr_size ){
    uverify( id_valid( id ) );
    uverify( 0 < ptr_size );

    printf( "\nqemdlink<0x%08x> len=%d\n", id, qemdlink_len( id ) );
    const int num = qemdlink_for_each( id, qemdlink_printf_callback, (void*)ptr_size );
    printf( "end(total %d)-------\n", num );
    return true;
}
#endif



typedef struct{
    node_t node;
    qemdlink_ptr_to_node_t ptr_to_node;
    qemdlink_node_to_ptr_t node_to_ptr;
}head_t;


static node_t* qemdlink_head_node( int id ){
    uverify( id_valid( id ) );

    head_t* const head = (head_t*)id;
    node_t* const node = &head->node;
    return node;
}






// qemdlink id，就是链表的head指针
int qemdlink_open( qemdlink_ptr_to_node_t ptr_to_node, qemdlink_node_to_ptr_t node_to_ptr ){
    uverify( NULL != ptr_to_node );
    uverify( NULL != node_to_ptr );

    head_t* const head = malloc( sizeof( head_t ) );
    return_0_if( NULL == head );

    head->ptr_to_node = ptr_to_node;
    head->node_to_ptr = node_to_ptr;

    node_t* const node = &head->node;
    node->next = node;
    node->prev = node;

    return (int)head;
}
// you must sure all nodes have been remove before call close()
// because some nodes may be on stack, so you cann't call remove_all()
bool qemdlink_close( int id ){
    uverify( id_valid( id ) );
    uverify( qemdlink_empty( id ) );

    // needn't remove all node's, because node's memory control by user code space.
    // Especialy, some node maybe on stack
    head_t* const head = (head_t*)id;
    //if( GCFG_DEBUG_EN ){
        memset( head, 0, sizeof( head_t ) );
    //}
    free( head );

    return true;
}




qemdlink_node_t* qemdlink_node( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );

    const head_t* const head = (head_t*)id;
    return head->ptr_to_node( ptr );
}
void* qemdlink_ptr( int id, qemdlink_node_t* node ){
    uverify( id_valid( id ) );
    uverify( NULL != node );

    const head_t* const head = (head_t*)id;
    return head->node_to_ptr( node );
}




bool qemdlink_node_init( qemdlink_node_t* node ){
    uverify( NULL != node );

    const node_t the_default = QEMDLINK_NODE_DEFAULT;
    *node = the_default;
    return true;
}
bool qemdlink_ptr_init( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr ) );

    qemdlink_node_t* const node = qemdlink_node( id, ptr );
    return_false_if( !qemdlink_node_init( node ) );
    return true;
}



// return: remove node num
int qemdlink_remove_all( int id ){
    uverify( id_valid( id ) );

    head_t* const head = (head_t*)id;
    node_t* const node = &head->node;
    // p pointer to node
    // q pointer to node->next
    node_t* p = NULL;
    node_t* q = NULL;
    int cnt = 0;
    for( p=node->next; p!=node; p=q, cnt++ ){
        q = p->next;
        // 如果在调试状态，将指针清零，便于侦测到错误
        //if( GCFG_DEBUG_EN ){
            memset( p, 0, sizeof( node_t ) );
       // }
        // 内存的申请、释放，不由本模块负责。
    }

    node->next = node;
    node->prev = node;

    return cnt;
}





//typedef bool (*qemdlink_for_eack_t)( int node_id, void* arg );
// return: loop time
int qemdlink_for_each( int id, qemdlink_for_eack_t callback, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != callback );

    if( qemdlink_empty( id ) ){
        return 0;
    }

    head_t* const head = (head_t*)id;
    node_t* const node = &head->node;
    // p pointer to node
    node_t* p = NULL;
    node_t* q = NULL;
    int ix = 0;
    for( p=node->next; p!=node; p=q, ix++ ){
        q = p->next;
        void* const ptr = qemdlink_ptr( id, p );
        if( !callback( id, ptr, ix, arg ) ){
            return ix + 1;
        }
    }

    return ix;
}




static bool qemdlink_len_callback( int id, void* ptr, int ix, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );
    uverify( ix_valid( ix ) );
    uverify( NULL == arg );
    return true;
}
int qemdlink_len( int id ){
    uverify( id_valid( id ) );

    const int loop_cnt = qemdlink_for_each( id, qemdlink_len_callback, NULL );
    return loop_cnt;
}




bool qemdlink_empty( int id ){
    uverify( id_valid( id ) );

    const node_t* const head_node = qemdlink_head_node( id );
    if( head_node == head_node->next ){
        uverify( head_node == head_node->prev );
        return true;
    }

    return false;
}




// return; ptr
void* qemdlink_first( int id ){
    uverify( id_valid( id ) );
    
    if( qemdlink_empty( id ) ){
        return 0;
    }

    const node_t* const head_node = qemdlink_head_node( id );
    node_t* const first_node = head_node->next;

    void* const first_ptr = qemdlink_ptr( id, first_node );
    return first_ptr;
}







// return; ptr
void* qemdlink_last( int id ){
    uverify( id_valid( id ) );

    if( qemdlink_empty( id ) ){
        return 0;
    }

    const node_t* const head_node = qemdlink_head_node( id );
    node_t* const last_node = head_node->prev;

    void* const last_ptr = qemdlink_ptr( id, last_node );
    return last_ptr;
}




// here -> next
static node_t* qemdlink_next_node( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr ) );

    const node_t* const here = qemdlink_node( id, ptr );
    node_t* const next = here->next;
    return next;
}





// here -> next
// if next is the last node, will return 0
// return: ptr
void* qemdlink_next( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );

    node_t* const next = qemdlink_next_node( id, ptr );

    // last node?
    node_t* const head_node = qemdlink_head_node( id );
    if( next == head_node ){
        return 0;
    }

    void* const next_ptr = qemdlink_ptr( id, next );
    return next_ptr;
}




// prev -> here
static node_t* qemdlink_prev_node( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr ) );

    const node_t* const here = qemdlink_node( id, ptr );
    node_t* const prev = here->prev;
    return prev;
}




// prev -> here
// if prev is the first node, will return 0
// return: ptr
void* qemdlink_prev( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );

    node_t* const prev = qemdlink_prev_node( id, ptr );

    // first node?
    node_t* const head_node = qemdlink_head_node( id );
    if( prev == head_node ){
        return 0;
    }

    void* const prev_ptr = qemdlink_ptr( id, prev );
    return prev_ptr;
}






typedef struct{
    const int ix;
    void* result_ptr;
}at_callback_t;
// find: node->next == obj, return node
static bool qemdlink_at_callback( int id, void* ptr, int ix, void* void_arg ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );
    uverify( ix_valid( ix ) );
    at_callback_t* const arg = (at_callback_t*)void_arg;
    uverify( ix_valid( arg->ix ) );
    uverify( NULL == arg->result_ptr );

    if( ix == arg->ix ){
        arg->result_ptr = ptr;
        return false;
    }

    return true;
}
// return: ptr
void* qemdlink_at( int id, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    at_callback_t arg = { ix, NULL };
    qemdlink_for_each( id, qemdlink_at_callback, &arg );
    return arg.result_ptr;
}





typedef struct{
    int ix;
    const void* const ptr;
}ix_callback_t;
// find: node->next == obj, return node
static bool qemdlink_ix_callback( int id, void* ptr, int ix, void* void_arg ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );
    uverify( ix_valid( ix ) );
    ix_callback_t* const arg = (ix_callback_t*)void_arg;
    uverify( ptr_valid( arg->ptr ) );

    if( ptr == arg->ptr ){
        uverify( 0 > arg->ix );
        arg->ix = ix;
        return false;
    }

    return true;
}
// return: index of ptr in link. if ptr not exist in link, return <0.
int qemdlink_ix( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr ) );

    ix_callback_t arg = { -1, ptr };
    qemdlink_for_each( id, qemdlink_ix_callback, &arg );
    return arg.ix;
}



bool qemdlink_in( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr ) );

    qemdlink_node_t* const node = qemdlink_node( id, ptr );
    if( NULL == node->next ){
        uverify( NULL == node->prev );
        return false;
    }

    uverify( NULL != node->prev );
    return true;
}



// here(node) -> next
// =>
// here(node) -> src -> next
// return: ok?
static bool qemdlink_append_node( node_t* here_node, node_t* src_node ){
    uverify( NULL != here_node );
    uverify( NULL != src_node );

    uverify( NULL != here_node->next );
    uverify( NULL != here_node->prev );

    uverify( NULL == src_node->next );
    uverify( NULL == src_node->prev );

    node_t* const next = here_node->next;

    here_node->next = src_node;
    src_node->next = next;

    next->prev = src_node;
    src_node->prev = here_node;
    return true;
}



// here -> next
// =>
// here -> src -> next
// return; ok?
bool qemdlink_append( int id, void* here, void* src ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( here ) );
    uverify( ptr_valid( src ) );

    node_t* const here_node = qemdlink_node( id, here );
    node_t* const src_node = qemdlink_node( id, src );
    return_false_if( !qemdlink_append_node( here_node, src_node ) );
    return true;
}



static node_t* qemdlink_last_node( int id ){
    uverify( id_valid( id ) );

    node_t* const head_node = qemdlink_head_node( id );
    if( qemdlink_empty( id ) ){
        return head_node;
    }

    return head_node->prev;
}
// last->null
// =>
// last->src->null
// return: ok?
bool qemdlink_append_last( int id, void* src ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( src ) );

    node_t* const last_node = qemdlink_last_node( id );
    uverify( NULL != last_node );

    node_t* const src_node = qemdlink_node( id, src );
    return_false_if( !qemdlink_append_node( last_node, src_node ) );
    return true;
}








// prev -> here
// =>
// prev -> src -> here
// return: ok?
bool qemdlink_insert( int id, void* here, void* src ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( here ) );
    uverify( ptr_valid( src ) );

    node_t* const prev_node = qemdlink_prev_node( id, here );
    uverify( NULL != prev_node );

    node_t* const src_node = qemdlink_node( id, src );
    return_false_if( !qemdlink_append_node( prev_node, src_node ) );
    return true;
}






// prev -> node -> next
// =>
// prev -> next
// return: ok?
// 这里的remove,和qlink & qdlink 的delete含义不一样：
// 只将内存块从链表中拆卸下来，不负责释放内存
bool qemdlink_remove( int id, void* ptr ){
    uverify( id_valid( id ) );
    uverify( ptr_valid( ptr_valid ) );

    node_t* const prev_node = qemdlink_prev_node( id, ptr );
    // not found the node
    return_false_if( NULL == prev_node );

    node_t* const ptr_node = qemdlink_node( id, ptr );
    node_t* const next = ptr_node->next;

    prev_node->next = next;
    next->prev = prev_node;

    //if( GCFG_DEBUG_EN ){
        ptr_node->next = NULL;
        ptr_node->prev = NULL;
    //}
    // 内存的释放，不由本模块负责
    return true;
}







// 单元测试例子及模板
#if GCFG_UNIT_TEST_EN > 0
typedef struct{
    char ch;
    node_t node;
}test_ptr_t;
static qemdlink_node_t* test_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    test_ptr_t* const p = (test_ptr_t*)ptr;
    return &(p->node);
}
static void* test_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, test_ptr_t, node );
    return p;
}
#if 0 
static bool node_printf_test( int id, void* ptr, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    if( 0 == ptr ){
        printf( "%d <NULL>", ix );
        return true;
    }

    const char ch = ((test_ptr_t*)ptr)->ch;
    const node_t* const node = qemdlink_node( id, ptr );
    const node_t* const next = node->next;
    const node_t* const prev = node->prev;
    printf( "%d ch=%c node=0x%08x next=0x%08x prev=0x%08x\n", ix, ch, (int)node, (int)next, (int)prev );
    return true;
}
#endif




static bool test_open_close(void){
    int id = qemdlink_open( test_ptr_to_node, test_node_to_ptr );
    test( qemdlink_close( id ) );
    id = 0;
    return true;
}



static bool test_1_node(void){
    // head
    int id = qemdlink_open( test_ptr_to_node, test_node_to_ptr );

    {
        test( qemdlink_empty( id ) );
        test( 0 == qemdlink_len( id ) );

        test( NULL == qemdlink_first( id ) );
        test( NULL == qemdlink_last( id ) );
        test( NULL == qemdlink_at( id, 0 ) );
    }

    {
        const test_ptr_t n0_const = { '0', QEMDLINK_NODE_DEFAULT };
        test_ptr_t* const n0 = malloc( sizeof(test_ptr_t) );
        *n0 = n0_const;
        test( qemdlink_append_last( id, n0 ) );
        // head->n0

        test( !qemdlink_empty( id ) );
        test( 1 == qemdlink_len( id ) );

        test_ptr_t* const first = qemdlink_first( id );
        test( n0 == first );
        test_ptr_t* const last = qemdlink_last( id );
        test( n0 == last );

        test( NULL == qemdlink_next( id, first ) );
        test( NULL == qemdlink_prev( id, first ) );
        
        test( n0 == qemdlink_at( id, 0 ) );
        test( NULL == qemdlink_at( id, 1 ) );

        test( qemdlink_remove( id, n0 ) );
        free( n0 );
        // head

        test( qemdlink_empty( id ) );
        test( 0 == qemdlink_len( id ) );

    }

    // append var on stack
    {
        test_ptr_t n1 = { '1', QEMDLINK_NODE_DEFAULT };
        test( qemdlink_append_last( id, &n1 ) );
        // head->n1

        test( !qemdlink_empty( id ) );
        test( 1 == qemdlink_len( id ) );

        // needn't remove n1, because it's on stack
        test( qemdlink_remove( id, &n1 ) );
    }

    test( qemdlink_close( id ) );
    id = 0;
    return true;
}



static bool test_2_node(void){
    int id = qemdlink_open( test_ptr_to_node, test_node_to_ptr );
    // head

    // append last
    {
        test_ptr_t* const n0 = malloc( sizeof(test_ptr_t) );
        test( qemdlink_node_init( &n0->node ) );
        n0->ch = '0';
        test( qemdlink_append_last( id, n0 ) );
        // head->n0
    }
    {
        const test_ptr_t n1_const = { '1', QEMDLINK_NODE_DEFAULT };
        test_ptr_t* const n1 = malloc( sizeof(test_ptr_t) );
        *n1 = n1_const;
        test( qemdlink_append_last( id, n1 ) );
        // head->n0->n1
    }
    {
        test( !qemdlink_empty( id ) );
        test( 2 == qemdlink_len( id ) );

        test_ptr_t* const first = qemdlink_first( id );
        test( NULL != first );
        test( '0' == first->ch  );
        test_ptr_t* const last = qemdlink_last( id );
        test( NULL != last );
        test( first != last );
        test( '1' == last->ch );

        test( last == qemdlink_next( id, first ) );
        test( first == qemdlink_prev( id, last ) );
        
        test( first == qemdlink_at( id, 0 ) );
        test( last == qemdlink_at( id, 1 ) );
        test( NULL == qemdlink_at( id, 2 ) );
    }

    {
        test_ptr_t* const n0 = qemdlink_first( id );
        test( qemdlink_remove( id, n0 ) );
        free( n0 );
        // head->n1

        test( !qemdlink_empty( id ) );
        test( 1 == qemdlink_len( id ) );
    }

    // insert
    {
        test_ptr_t* const n1 = qemdlink_last( id );
        test( '1' == n1->ch );

        const test_ptr_t n0_const = { '0', QEMDLINK_NODE_DEFAULT };
        test_ptr_t* const n0 = malloc( sizeof( test_ptr_t ) );
        *n0 = n0_const;
        test( qemdlink_insert( id, n1, n0 ) );
        // head->n0->n1

        test( !qemdlink_empty( id ) );
        test( 2 == qemdlink_len( id ) );
    }

    {
        test_ptr_t* const n1 = qemdlink_last( id );
        test( '1' == n1->ch );
        test( qemdlink_remove( id, n1 ) );
        free( n1 );
        // head->n0
    }

    // append
    {
        test_ptr_t* const n1 = malloc( sizeof( test_ptr_t ) );
        const test_ptr_t n1_const = { '1', QEMDLINK_NODE_DEFAULT };
        *n1 = n1_const;

        test_ptr_t* const n0 = qemdlink_at( id, 0 );
        test( '0' == n0->ch );
        test( qemdlink_append( id, n0, n1 ) );
        // head->n0->n1
    }

    // remove all
    {
        test_ptr_t* const n0 = qemdlink_at( id, 0 );
        test( '0' == n0->ch );
        test_ptr_t* const n1 = qemdlink_at( id, 1 );
        test( '1' == n1->ch );
        test( 2 == qemdlink_remove_all( id ) );
        free( n0 );
        free( n1 );

        test( qemdlink_empty( id ) );
        test( 0 == qemdlink_len( id ) );
    }

    test( qemdlink_close( id ) );
    id = 0;

    return true;
}




static bool qemdlink_delete_all_callback( int id, void* ptr, int ix, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != ptr );
    uverify( ix_valid( ix ) );
    uverify( NULL == arg );
    
    return_false_if( !qemdlink_remove( id, ptr ) );
    qfree( ptr );

    return true;
}
static bool test_for_each_with_free(void){
    int id = qemdlink_open( test_ptr_to_node, test_node_to_ptr );

    {
        test_ptr_t* const n0 = qmalloc( sizeof(test_ptr_t) );
        test( qemdlink_node_init( &n0->node ) );
        n0->ch = '0';
        test( qemdlink_append_last( id, n0 ) );
        // head->n0
    }
    {
        test_ptr_t* const n1 = qmalloc( sizeof(test_ptr_t) );
        test( qemdlink_node_init( &n1->node ) );
        n1->ch = '1';
        test( qemdlink_append_last( id, n1 ) );
        // head->n0->n1
    }
    {
        test_ptr_t* const n2 = qmalloc( sizeof(test_ptr_t) );
        test( qemdlink_node_init( &n2->node ) );
        n2->ch = '2';
        test( qemdlink_append_last( id, n2 ) );
        // head->n0->n1->n2
    }

    test( 3 == qemdlink_len( id ) );
    test( 3 == qemdlink_for_each( id, qemdlink_delete_all_callback, NULL ) );

    test( qemdlink_close( id ) );
    id = 0;
    return true;
}

// unit test entry, add entry in file_list.h
bool unit_test_qemdlink(void){
    const unit_test_item_t table[] = {
        test_open_close
        , test_1_node
        , test_2_node
        , test_for_each_with_free
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif



// no more------------------------------
