/*
 * File      : qmem.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-04     huanglin     first version
 *
 */

/*
qmem function:
1    position 
2    count 
3    size 
4    edge check

mem: mem_info_t edge_left(4) ptr(size) edge_right(4)
 */



//{{{ include
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "stdlib.h"


#include "inc.h"
#include "qemdlink.h"
#include "qmem.h"


#define this_file_id   file_id_qmem


static bool qmem_init(void);
// }}}



//{{{ mem_log

// log the mem information
static int m_link = 0;
typedef struct {
    mem_info_t mem_info;
    qemdlink_node_t node;
}mem_log_t;




static qemdlink_node_t* ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    mem_log_t* const p = (mem_log_t*)ptr;
    return &(p->node);
}
static void* node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, mem_log_t, node );
    return p;
}
static bool qmem_log_init(void){
    uverify( 0 == m_link );
    m_link = qemdlink_open( ptr_to_node, node_to_ptr );
    return_false_if( 0 == m_link );
    return true;
}




static bool qmem_info_log( const mem_info_t* mem_info ){
    uverify( NULL != mem_info );
    uverify( NULL != mem_info->ptr );

    mem_log_t* const mem_log = malloc( sizeof( mem_log_t ) );
    return_false_if( NULL == mem_log );
    return_false_if( !qemdlink_node_init( &mem_log->node ) );

    mem_log->mem_info = *mem_info;
    return_false_if( !qemdlink_append_last( m_link, mem_log ) );
    return true;
}





static bool qmem_info_remove( mem_info_t* mem_info ){
    uverify( NULL != mem_info );
    
    mem_log_t* const mem_log = container_of( mem_info, mem_log_t, mem_info );
    return_false_if( !qemdlink_remove( m_link, mem_log ) );
    free( mem_log );
    return true;
}



typedef struct{
    void* find_ptr;
    mem_info_t* result_mem_info;
}find_ptr_callback_t;
static bool find_ptr_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( 0 != link_id );
    mem_log_t* const mem_log = (mem_log_t*)ptr;
    uverify( NULL != mem_log );
    mem_info_t* const mem_info = &mem_log->mem_info;
    uverify( 0 <= ix );
    find_ptr_callback_t* const arg = (find_ptr_callback_t*)void_arg;
    uverify( NULL != arg );

    if( mem_info->ptr == arg->find_ptr ){
        uverify( NULL == arg->result_mem_info );
        arg->result_mem_info = mem_info;
        return false;
    }

    return true;
}
// return: mem_info
mem_info_t* ptr_to_mem_info( void* ptr ){
    uverify( NULL != ptr );

    find_ptr_callback_t arg = { ptr, NULL };
    qemdlink_for_each( m_link, find_ptr_callback, &arg );
    return arg.result_mem_info;
}




bool qmem_info( mem_info_t* mem_info, void* ptr ){
    if( 0 == GCFG_MEM_EN ){
        return true;
    }

    uverify( NULL != mem_info );
    uverify( NULL != ptr );

    const mem_info_t* const mem_info_of_ptr = ptr_to_mem_info( ptr );
    if( NULL == mem_info_of_ptr ){
        return false;
    }
    *mem_info = *mem_info_of_ptr;
    return true;
}




int qmem_count(void){
    if( 0 == GCFG_MEM_EN ){
        return 0;
    }

    return_0_if( !qmem_init() );
    return qemdlink_len( m_link );
}



typedef struct{
    qmem_for_eack_t callback;
    void* arg;
}qmem_for_eack_callback_t;
static bool qmem_for_eack_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( qemdlink_id_valid( link_id ) );
    const mem_log_t* const mem_log = (mem_log_t*)ptr;
    uverify( NULL != mem_log );
    const mem_info_t* const mem_info = &mem_log->mem_info;
    uverify( 0 <= ix );
    const qmem_for_eack_callback_t* const arg = (qmem_for_eack_callback_t*)void_arg;
    uverify( NULL != arg );
    qmem_for_eack_t callback = arg->callback;
    uverify( NULL != callback );

    if( !callback( mem_info, ix, arg->arg ) ){
        return false;
    }

    return true;
}
// return: loop mem cnt
int qmem_for_each( qmem_for_eack_t callback, void* arg ){
    if( 0 == GCFG_MEM_EN ){
        return 0;
    }

    return_0_if( !qmem_init() );
    qmem_for_eack_callback_t callback_arg = { callback, arg };
    return qemdlink_for_each( m_link, qmem_for_eack_callback, &callback_arg );
}






//}}}



//{{{ edge

// edge pattern
static const char m_edge_left_pattern[] = { 'e', 'd', 'g', 'L' };
static const int m_edge_left_len = sizeof( m_edge_left_pattern );
static const char m_edge_right_pattern[] = { 'e', 'd', 'g', 'R' };
static const int m_edge_right_len = sizeof( m_edge_right_pattern );



static void* edge_left_by_ptr( void* ptr ){
    uverify( NULL != ptr );
    return ptr - m_edge_left_len;
}
static void* edge_right_by_ptr( void* ptr, size_t size ){
    uverify( NULL != ptr );
    return ptr + size;
}
static bool edge_init( void* edge, const char* pattern, int pattern_len ){
    uverify( NULL != edge );
    uverify( NULL != pattern );
    uverify( 0 <= pattern_len );

    memcpy( edge, pattern, pattern_len );
    return true;
}
static bool edge_check( void* edge, const char* pattern, int pattern_len ){
    uverify( NULL != edge );
    uverify( NULL != pattern );
    uverify( 0 <= pattern_len );

    if( 0 != memcmp( edge, pattern, pattern_len ) ){
        return false;
    }

    return true;
}



bool qmem_edge( qmem_edge_t* edge, void* ptr ){
    if( 0 == GCFG_MEM_EN ){
        return true;
    }

    uverify( NULL != edge );
    uverify( NULL != ptr );

    const mem_info_t* const mem_info = ptr_to_mem_info( ptr );
    return_false_if( NULL == mem_info );

    edge->left = edge_left_by_ptr( ptr );
    edge->left_pattern = m_edge_left_pattern;
    edge->left_len = m_edge_left_len;

    edge->right = edge_right_by_ptr( ptr, mem_info->size );
    edge->right_pattern = m_edge_right_pattern;
    edge->right_len = m_edge_right_len;

    return true;
}




bool qmem_check( void* ptr ){
    if( 0 == GCFG_MEM_EN ){
        return true;
    }

    uverify( NULL != ptr );

    const mem_info_t* const mem_info = ptr_to_mem_info( ptr );
    return_false_if( NULL == mem_info );

    void* const edge_left = edge_left_by_ptr( ptr );
    void* const edge_right = edge_right_by_ptr( ptr, mem_info->size );

    if( !edge_check( edge_left, m_edge_left_pattern, m_edge_left_len ) ){
        return false;
    }
    if( !edge_check( edge_right, m_edge_right_pattern, m_edge_right_len ) ){
        return false;
    }

    return true;
}


static bool qmem_check_all_callback( const mem_info_t* mem_info, int ix, void* arg ){
    uverify( NULL != mem_info );
    uverify( 0 <= ix );
    // if check fail, store the mem information
    mem_info_t* const mem_info_fail = (mem_info_t*)arg;
    uverify( NULL != mem_info_fail );

    if( !qmem_check( mem_info->ptr ) ){
        uverify( NULL == mem_info_fail->ptr );
        *mem_info_fail = *mem_info;
        return false;
    }
    return true;
}
// return: ok? if check fail, mem store the 1st error mem infomation.
bool qmem_check_all( mem_info_t* mem_info_fail ){
    if( 0 == GCFG_MEM_EN ){
        return true;
    }

    uverify( NULL != mem_info_fail );

    memset( mem_info_fail, 0, sizeof(mem_info_t) );
    qmem_for_each( qmem_check_all_callback, mem_info_fail );
    if( NULL != mem_info_fail->ptr ){
        return false;
    }
    return true;
}




//}}}



//{{{ malloc & free


static int mem_size_with_edge( int size ){
    uverify( 0 <= size );
    return m_edge_left_len + size + m_edge_right_len;
}



static bool init_edge( void* ptr, int size ){
    uverify( NULL != ptr );
    uverify( 0 <= size );

    void* const edge_left = edge_left_by_ptr( ptr );
    void* const edge_right = edge_right_by_ptr( ptr, size );

    return_false_if( !edge_init( edge_left, m_edge_left_pattern, m_edge_left_len ) );
    return_false_if( !edge_init( edge_right, m_edge_right_pattern, m_edge_right_len ) );
    return true;
}

static void* malloc_with_edge( size_t size ){
    const int real_size = mem_size_with_edge( size );
    // edge mem edge
    void* ptr = malloc( real_size );
    return_0_if( NULL == ptr );

    return ptr + m_edge_left_len;
}
// ptr 和 mem_info_link 本来可以共用一块内存，调用一次malloc()即可。
// 但考虑到内存越界写的可能，因此，用两次malloc()，使ptr和mem_info_link
// 尽量不在一起，对mem_info_link被损坏的可能性减小，侦测出错误的可能性增大。
void* qmalloc_real( int file_id, int line, size_t size ){
    if( 0 == GCFG_MEM_EN ){
        return malloc( size );
    }

    return_0_if( !qmem_init() );

    uverify( 0 <= line );
    uverify( 0 <= size );

    void* const ptr = malloc_with_edge( size );
    return_0_if( NULL == ptr );
    static int malloc_exec_ix = 0;
    ++malloc_exec_ix;

    return_0_if( !init_edge( ptr, size ) );

    const mem_info_t mem_info = { malloc_exec_ix, file_id, line, ptr, size };
    return_0_if( !qmem_info_log( &mem_info ) );

    return ptr;
}




static void* realloc_with_edge( void* ptr, int newsize ){
    uverify( NULL != ptr );
    uverify( 0 <= newsize );

    void* const edge_left = edge_left_by_ptr( ptr );
    const int real_size = mem_size_with_edge( newsize );
    // edge mem edge
    void* const new_ptr = realloc( edge_left, real_size );
    return_0_if( NULL == new_ptr );
    
    return new_ptr + m_edge_left_len;
}
void* qrealloc_real( int file_id, int line, void *ptr, int newsize ){
    if( 0 == GCFG_MEM_EN ){
        return realloc( ptr, newsize );
    }

    return_0_if( !qmem_init() );

    // If a null pointer is passed for$ptr$,$realloc()$behaves just like malloc(newsize)
    if( NULL == ptr ){
        return qmalloc_real( file_id, line, newsize );
    }

    uverify( 0 <= line );
    uverify( NULL != ptr );
    uverify( 0 <= newsize );

    uverify( qmem_check( ptr ) );
    mem_info_t* const mem_info_of_ptr = ptr_to_mem_info( ptr );
    return_0_if( NULL == mem_info_of_ptr );

    void* const new_ptr = realloc_with_edge( ptr, newsize );
    return_0_if( NULL == new_ptr );
    return_0_if( !init_edge( new_ptr, newsize ) );

    // just modify mem info
    const mem_info_t mem_info = { mem_info_of_ptr->malloc_exec_ix, file_id, line, new_ptr, newsize };
    *mem_info_of_ptr = mem_info;
    return new_ptr;
}


static bool destory_mem_edge( void* ptr ){
    uverify( NULL != ptr );
    qmem_edge_t mem_edge;
    return_false_if( !qmem_edge( &mem_edge, ptr ) );

    memset( mem_edge.left, 0, mem_edge.left_len );
    memset( mem_edge.right, 0, mem_edge.right_len );
    return true;
}



// do NOT check mem in qfree(), because free is free, check is check.
// remove mem info
// free ptr
bool qfree_real( int file_id, int line, void* ptr ){
    if( 0 == GCFG_MEM_EN ){
        free( ptr );
        return true;
    }

    return_false_if( !qmem_init() );

    uverify( 0 <= line );
    uverify( NULL != ptr );
    //uverify( qmem_check( ptr ) );

    mem_info_t* const mem_info = ptr_to_mem_info( ptr );
    // NOT found?
    if( NULL == mem_info ){
        return false;
    }

    // if the ptr use after free, because edge destoryed, qmem_check() will be failed.
    return_false_if( !destory_mem_edge( ptr ) );

    if( !qmem_info_remove( mem_info ) ){
        return false;
    }

    void* const edge_left = edge_left_by_ptr( ptr );
    free( edge_left );
    return true;
}




// }}}



//{{{ init
static bool qmem_init(void){
    static bool run = false;
    if( run ){
        return true;
    }
    run = true;

    return_false_if( !qmem_log_init() );
    return true;
}
//}}}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0

typedef struct{
    int a;
    int b;
    char c;
}test_t;

static bool test_offsetof(void){
    test( 0 == offsetof( test_t, a ) );
    test( 4 == offsetof( test_t, b ) );
    test( 8 == offsetof( test_t, c ) );
    return true;
}


static bool test_container_of(void){
    test_t t = { 1, 2 , 3 };
    const int* pb = &t.b;

    const test_t* const pt = container_of( pb, test_t, b );
    test( pt == &t );

    return true;
}



static bool test_log(void){
    const int count = qmem_count();

    {
        char* const p = qmalloc( 5 );
        const int line = __LINE__;
        test( count+1 == qmem_count() );
        mem_info_t mem_info;
        test( qmem_info( &mem_info, p ) );
        test( mem_info.file_id == this_file_id );
        test( mem_info.line == line - 1 );
        test( mem_info.ptr == p );
        test( mem_info.size == 5 );
        qfree( p );

        test( count+0 == qmem_count() );
        test( !qmem_info( &mem_info, p ) );
    }

    {
        char* const p = qmalloc( 6 );
        const int line_p = __LINE__;
        test( count+1 == qmem_count() );
        char* const q = qmalloc( 7 );
        const int line_q = __LINE__;
        test( count+2 == qmem_count() );

        mem_info_t mem_info;
        test( qmem_info( &mem_info, p ) );
        test( mem_info.file_id == this_file_id );
        test( mem_info.line == line_p - 1 );
        test( mem_info.ptr == p );
        test( mem_info.size == 6 );

        test( qmem_info( &mem_info, q ) );
        test( mem_info.file_id == this_file_id );
        test( mem_info.line == line_q - 1 );
        test( mem_info.ptr == q );
        test( mem_info.size == 7 );

        qfree( q );
        test( count+1 == qmem_count() );
        qfree( p );
        test( count+0 == qmem_count() );
    }

    test( count+0 == qmem_count() );
    return true;
}




static bool test_edge(void){
    // right edge
    {
        char* const p = qmalloc( 5 );
        test( qmem_check( p ) );
        memcpy( p, "12345", 5 );
        test( qmem_check( p ) );
        memcpy( p, "123456", 6 );
        test( !qmem_check( p ) );

        // qmem_edge
        qmem_edge_t edge;
        test( qmem_edge( &edge, p ) );
        test( '6' == *(edge.right) );

        qfree( p );
    }

    // left edge
    {
        char* const p = qmalloc( 6 );
        test( qmem_check( p ) );
        memcpy( p, "12345", 5 );
        test( qmem_check( p ) );
        char* const p_1 = p - 1;
        *p_1 = '0';
        test( !qmem_check( p ) );
        qfree( p );
    }

    // qmem_check_all
    {
        char* const p = qmalloc( 6 );
        char* const q = qmalloc( 7 );

        mem_info_t mem_info_fail;
        test( qmem_check_all( &mem_info_fail ) );

        memcpy( q, "12345678", 8 );
        test( !qmem_check_all( &mem_info_fail ) );
        test( mem_info_fail.ptr == q );

        char* const p_1 = p - 1;
        *p_1 = '0';
        test( !qmem_check_all( &mem_info_fail ) );
        test( mem_info_fail.ptr == p );

        qfree( p );
        qfree( q );
    }

    return true;
}




static bool test_realloc(void){
    char* const p = qmalloc( 5 );
    test( NULL != p );

    memcpy( p, "12345", 5 );
    
    
    // realloc shrink memory
    char* p1 = qrealloc( p, 3 );
    test( NULL != p1 );
    test( 0 == memcmp( p1, "123", 3 ) );
    test( qmem_check( p1 ) );

    {
        mem_info_t mem_info;
        test( qmem_info( &mem_info, p1 ) );
        test( mem_info.size == 3 );
    }

    // realloc increase memory
    char* p2 = qrealloc( p1, 8 );
    test( NULL != p2 );
    test( 0 == memcmp( p2, "123", 3 ) );
    test( qmem_check( p2 ) );

    {
        mem_info_t mem_info;
        test( qmem_info( &mem_info, p2 ) );
        test( mem_info.size == 8 );
    }

    qfree( p2 );
    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_qmem(void){
    const unit_test_item_t table[] = {
        test_offsetof
        , test_container_of
        , test_log
        , test_edge
        , test_realloc
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}



// no more------------------------------
