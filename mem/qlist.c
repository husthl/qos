/*
 * File      : qlist.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 * 一个支持动态增长的数组
 * 每个元素的长度相同
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
#include "qmem.h"
#include "qlist.h"

#define this_file_id   file_id_qlist


#define LIST_NBYTE_MIN      (128)
 

typedef struct node_s{
    int node_len;
    int node_num;

    int list_nbyte;
    char* list;
}qlist_t;

static bool id_valid( int id ){
    if( 0 == id ){
        return false;
    }

    const qlist_t* const qlist = (qlist_t*)id;
    if( 0 >= qlist->node_len ){
        return false;
    }
    if( 0 > qlist->node_num ){
        return false;
    }
    if( 0 > qlist->list_nbyte ){
        return false;
    }
    if( qlist->list_nbyte < qlist->node_len * qlist->node_num ){
        uverify( false );
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



// 因为qlist是个数组，因此，可以返回其首地址。这也是qlist相较qlink的优势所在。
// return: qlist pointer
const void* qlist_p( int id ){
    uverify( id_valid( id ) );

    qlist_t* const qlist = (qlist_t*)id;
    return qlist->list;
}




// node: a char, 1 byte len
static bool node_printf_default( int ix, void* node_p ){
    uverify( ix_valid( ix ) );

#if 0
    if( NULL == node_p ){
        printf( "%d <NULL>", ix );
        return true;
    }

    const char* const data = node_p;
    printf( "%d data=%c p=0x%08x\n", ix, *data, (int)data );
#endif
    return true;
}
static bool qlist_printf_callback( void* node_p, int ix, void* arg ){
    uverify( NULL != node_p );
    uverify( ix_valid( ix ) );
    qlist_node_printf_callback_t node_printf = (qlist_node_printf_callback_t)arg;
    
    if( !node_printf( ix, node_p ) ){
        return false;
    }

    return true;
}
// return: printf node num
int qlist_printf( int id, qlist_node_printf_callback_t callback ){
    uverify( 0 != id );
    if( NULL == callback ){
        callback = node_printf_default;
    }

    printf( "\nqlist<0x%08x>\n", id );
    qlist_t* const qlist = (qlist_t*)id;
    printf( "node_num=%d node_len=%d list<0x%08x> list_nbyte=%d\n", qlist->node_num, qlist->node_len, (int)(qlist->list), qlist->list_nbyte );

    const int num = qlist_for_each( id, qlist_printf_callback, callback );
    printf( "end(total %d)-------\n", num );
    return true;
}



// qlist id
int qlist_open( int node_len ){
    qlist_t* const qlist = qmalloc( sizeof( qlist_t ) );
    return_0_if( NULL == qlist );

    qlist->node_len = node_len;
    qlist->node_num = 0;
    qlist->list_nbyte = LIST_NBYTE_MIN;
    qlist->list = qmalloc( LIST_NBYTE_MIN );

    const int id = (int)qlist;
    uverify( id_valid( id ) );
    return id;
}



static bool delete_list( int id ){
    uverify( id_valid( id ) );

    qlist_t* const qlist = (qlist_t*)id;
    qlist->node_num = 0;
    qfree( qlist->list );
    qlist->list = NULL;
    qlist->list_nbyte = 0;
    return true;
}
bool qlist_close( int id ){
    uverify( id_valid( id ) );

    return_false_if( !delete_list( id ) );

    qlist_t* const qlist = (qlist_t*)id;
    if( GCFG_DEBUG_EN ){
        memset( qlist, 0, sizeof(qlist_t) );
    }
    qfree( qlist );

    return true;
}


/*
const void* qlist_list( int id ){
    uverify( id_valid( id ) );
    qlist_t* const qlist = (qlist_t*)id;
    return qlist->list;
}
*/




int qlist_node_offset( int id, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    qlist_t* const qlist = (qlist_t*)id;
    uverify( ix < qlist->node_num );

    return ( ix * (qlist->node_len) );
}




static void* qlist_node_at( int id, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    qlist_t* const qlist = (qlist_t*)id;
    uverify( ix < qlist->node_num );

    void* const p = qlist->list + qlist_node_offset( id, ix );
    return p;
}
void* qlist_at( int id, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    if( qlist_empty( id ) ){
        return 0;
    }

    qlist_t* const qlist = (qlist_t*)id;
    if( ix >= qlist->node_num ){
        return 0;
    }
    return qlist_node_at( id, ix );
}






//typedef bool (*qlist_for_eack_t)( int node_id, int ix, void* arg );
// return: loop time
int qlist_for_each( int id, qlist_for_eack_t callback, void* arg ){
    uverify( id_valid( id ) );

    qlist_t* const qlist = (qlist_t*)id;
    int i = 0;
    for( i=0; i<qlist->node_num; i++ ){
        void* const node_p = qlist_node_at( id, i );
        if( !callback( node_p, i, arg ) ){
            return i + 1;
        }
    }

    return i;
}




int qlist_len( int id ){
    uverify( id_valid( id ) );
    qlist_t* const qlist = (qlist_t*)id;
    return qlist->node_num;
}




bool qlist_empty( int id ){
    uverify( id_valid( id ) );
    qlist_t* const qlist = (qlist_t*)id;
    if( 0 == qlist->node_num ){
        return true;
    }

    return false;
}



/*
// return; node_id
int qlist_first( int id ){
    uverify( id_valid( id ) );

    if( qlist_empty( id ) ){
        return 0;
    }

    return qlist_node_at( id, 0 );
}







// return; node_id
int qlist_last( int id ){
    uverify( id_valid( id ) );

    if( qlink_empty( id ) ){
        return 0;
    }

    qlist_t* const qlist = (qlist_t*)id;
    uverify( 0 < qlist->node_num );
    const int last_ix = qlist->node_num - 1;
    return qlist_node_at( id, last_ix );
}
*/















#define max( a, b )     ( ( (a) > (b) ) ? (a) : (b) )
static bool qlist_grow_up_exec( int id, int grow_up_num ){
    uverify( id_valid( id ) );
    uverify( 0 <= grow_up_num );

    // everytime increase 10% at least.
    qlist_t* const qlist = (qlist_t*)id;
    const int grow_up_num_min = qlist->node_num * 10 / 100;
    const int grow_up_num_real = max( grow_up_num_min, grow_up_num );
    const int num = grow_up_num_real + qlist->node_num;
    //qlist_printf( id, NULL );
    //printf( "qlist_grow_up_exec(): qlist->node_num=%d grow_up_num_min=%d grow_up_num_real=%d num=%d\n", qlist->node_num, grow_up_num_min, grow_up_num_real, num );
    const int node_len = qlist->node_len;
    const int list_nbyte_new = num * node_len;
    uverify( list_nbyte_new > qlist->list_nbyte );
    char* const new_list = qrealloc( qlist->list, list_nbyte_new );

    // 如果没有足够的空间，老空间没有释放，仍然存在。简单返回false，说明grow_up失败。
    return_false_if( NULL == new_list );
    qlist->list = new_list;
    // 老空间内容，已经拷贝在前面，只用初始化后面一段即可。
    if( GCFG_DEBUG_EN ){
        memset( qlist->list + qlist->node_num * node_len, 0, grow_up_num_real * node_len );
    }

    qlist->node_num = qlist->node_num + grow_up_num;
    qlist->list_nbyte = list_nbyte_new;
    return true;

}
static bool qlist_grow_up( int id, int grow_up_num ){
    uverify( id_valid( id ) );
    uverify( 0 <= grow_up_num );

    if( 0 == grow_up_num ){
        return true;
    }

    qlist_t* const qlist = (qlist_t*)id;
    const int num_expect = grow_up_num + qlist->node_num;
    const int node_len = qlist->node_len;
    if( num_expect * node_len <= qlist->list_nbyte ){
        qlist->node_num = num_expect;
        return true;
    }

    return_false_if( !qlist_grow_up_exec( id, grow_up_num ) );
    return true;
}



/*
static bool qlist_grow_up_node_num( int id ){
    uverify( id_valid( id ) );
    return 10;
}
*/



/*
static bool qlist_grow_up( int id ){
    uverify( id_valid( id ) );


    qlist_t* const qlist = (qlist_t*)id;
    const int grow_up_node_num = qlist_grow_up_node_num( id );
    const int num = qlist->node_num;
    const int new_num = num + grow_up_node_num;

    return_false_if( !qlist_grow_up( id, grow_up_node_num ) );
    return true;
}
*/



static bool qlist_node_copy( int id, void* dst, const void* src ){
    uverify( id_valid( id ) );
    uverify( NULL != dst );
    uverify( NULL != src );

    qlist_t* const qlist = (qlist_t*)id;
    const int node_len = qlist->node_len;
    memcpy( dst, src, node_len );
    return true;
}




// 如果list长度不够，会增长到足够的长度
// 例如：list当前只有5个长，调用qlist_set( id, 10, node_src );
// qlist会自动增长到11个元素长，并将第10个拷贝成和node_src一样。
// node[0] node[1] ... node[ix] ... node[num]
// =>
// node[0] node[1] ... node_src ... node[num]
// return: ok?
bool qlist_set( int id, int ix, const void* src ){
    uverify( id_valid( id ) );
    uverify( 0 <= ix );
    uverify( NULL != src );

    qlist_t* const qlist = (qlist_t*)id;
    const int node_num = qlist->node_num;
    if( ix >= node_num ){
        const int new_num = ix + 1;
        const int grow_up_num = new_num - node_num;
        return_false_if( !qlist_grow_up( id, grow_up_num ) );
    }

    void* const dst = qlist_at( id, ix );
    return_false_if( !qlist_node_copy( id, dst, src ) );
    return true;
}



// return: ok
static bool qlist_node_move( int id, int to_ix, int from_ix, int num ){
    uverify( id_valid( id ) );
    uverify( 0 <= to_ix );
    uverify( 0 <= from_ix );
    uverify( 0 <= num );

    if( 0 == num ){
        return true;
    }

    qlist_t* const qlist = (qlist_t*)id;
    const int node_num = qlist->node_num;

    const int to_ix_end = to_ix + num - 1;
    // exceed list edge?
    return_false_if( to_ix_end + 1 > node_num );

    const int from_ix_end = from_ix + num - 1;
    // exceed list edge?
    return_false_if( from_ix_end + 1 > node_num );


    void* const to = qlist_at( id, to_ix );
    const void* const from = qlist_at( id, from_ix );

    const int len = qlist->node_len;
    memmove( to, from, len * num );
    return true;
}



// node[0] node[1] ... node[ix] ... node[num]
// =>
// node[0] node[1] ... node[ix] node_src ... node[num]
// return; ok?
bool qlist_append( int id, int ix, const void* src ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );
    uverify( NULL != src );
   
    qlist_t* const qlist = (qlist_t*)id;
    const int ori_node_num = qlist->node_num;

    if( ix < ori_node_num ){
        return_false_if( !qlist_grow_up( id, 1 ) );
    }
    else{
        const int num_min = ix + 1 + 1;
        if( num_min > ori_node_num ){
            const int grow_up_num = num_min - ori_node_num;
            return_false_if( !qlist_grow_up( id, grow_up_num ) );
        }
    }
    
    // node_num increase, so qlist->node_num != node_num
    uverify( qlist->node_num >= ix + 2 );
    return_false_if( !qlist_node_move( id, ix+2, ix+1, qlist->node_num - ix - 2 ) );
    return_false_if( !qlist_set( id, ix+1, src ) );

    return true;
}




// node[0] node[1] ... node[num]
// =>
// node[0] node[1] ... node[num] node_src
// return: ok?
bool qlist_append_last( int id, const void* src ){
    uverify( id_valid( id ) );
    uverify( NULL != src );

    return_false_if( !qlist_grow_up( id, 1 ) );

    qlist_t* const qlist = (qlist_t*)id;
    const int node_num = qlist->node_num;
    return_false_if( !qlist_set( id, node_num-1, src ) );

    return true;
}




// node[0] node[1] ... node[ix] ... node[num]
// =>
// node[0] node[1] ... node_src node[ix] ... node[num]
// return: ok?
bool qlist_insert( int id, int ix, const void* src ){
    uverify( id_valid( id ) );
    uverify( 0 <= ix );
    uverify( NULL != src );

    qlist_t* const qlist = (qlist_t*)id;
    const int ori_node_num = qlist->node_num;

    if( ix < ori_node_num ){
        return_false_if( !qlist_grow_up( id, 1 ) );
    }
    else{
        const int num_min = ix + 1 + 1;
        if( num_min > ori_node_num ){
            const int grow_up_num = num_min - ori_node_num;
            return_false_if( !qlist_grow_up( id, grow_up_num ) );
        }
    }

    // node_num increase, so qlist->node_num != node_num
    uverify( qlist->node_num >= ix + 2 );
    return_false_if( !qlist_node_move( id, ix+1, ix, qlist->node_num - ix - 2 ) );
    return_false_if( !qlist_set( id, ix, src ) );

    return true;
}




// node[0] node[1] ... node[ix] ... node[num]
// =>
// node[0] node[1] ...  ... node[num]
// return: ok?
bool qlist_remove( int id, int ix ){
    uverify( id_valid( id ) );
    uverify( ix_valid( ix ) );

    qlist_t* const qlist = (qlist_t*)id;
    const int node_num = qlist->node_num;
    if( ix >= node_num ){
        return false;
    }

    // remove last one
    // in fact, last element NOT erase, still exist in memory, just decrease the node_num.
    if( ix == node_num-1 ){
        qlist->node_num = node_num - 1;
    }

    if( !qlist_node_move( id, ix, ix+1, node_num - ix - 1 ) ){
        return false;
    }
    qlist->node_num = node_num - 1;

    return true;
}


// not relase the memory, just set node_num zero.
// the shortcoming is the memory always increase, NOT decrease.
// once the memory increase very large, it don't release untill qlist_close().
// return: ok?
bool qlist_remove_all( int id ){
    uverify( id_valid( id ) );

    qlist_t* const qlist = (qlist_t*)id;
    qlist->node_num = 0;
    return true;
}



// make list_nbyte = node_num * node_len. So decrease memory size.
// return: ok?
bool qlist_mem_fit( int id ){
    uverify( id_valid( id ) );
    qlist_t* const qlist = (qlist_t*)id;

    const int all_node_nbyte = qlist->node_num * qlist->node_len;
    const int list_nbyte_malloc = max( LIST_NBYTE_MIN, all_node_nbyte );
    char* const list_fit = qmalloc( list_nbyte_malloc );
    return_false_if( NULL == list_fit );
    if( GCFG_DEBUG_EN ){
        memset( list_fit, 0, list_nbyte_malloc );
    }
    memcpy( list_fit, qlist->list, all_node_nbyte );

    qfree( qlist->list );
    qlist->list = list_fit;
    qlist->list_nbyte = list_nbyte_malloc;
    return true;
}




// 单元测试例子及模板
#if GCFG_UNIT_TEST_EN > 0



static bool test_open_close(void){
    int id = qlist_open( sizeof( char ) );
    test( qlist_close( id ) );
    id = 0;
    return true;
}



static bool test_1_node(void){
    int id = qlist_open( sizeof( char ) );
    // list: 

    {
        test( qlist_empty( id ) );
        test( 0 == qlist_len( id ) );
        test( NULL == qlist_at( id, 0 ) );
        test( !qlist_remove( id, 0 ) );
    }

    // set
    {
        test( qlist_set( id, 0, "0" ) );
        // list: 0

        test( !qlist_empty( id ) );
        test( 1 == qlist_len( id ) );
        test( '0' == (*(char*)qlist_at( id, 0 )) );

        test( qlist_remove( id, 0 ) );
        // list:
        test( qlist_empty( id ) );
        test( 0 == qlist_len( id ) );
    }

    // set
    {
        test( qlist_set( id, 2, "2" ) );
        // list: 0 0 2
        test( !qlist_empty( id ) );
        test( 3 == qlist_len( id ) );
        test( '2' == (*(char*)qlist_at( id, 2 )) );
        test( qlist_remove( id, 0 ) );
        // list: 0 2
        test( qlist_remove( id, 0 ) );
        // list: 2 
        test( '2' == (*(char*)qlist_at( id, 0 )) );
        test( NULL == qlist_at( id, 1 ) );
        // list: 2
        test( '2' == (*(char*)qlist_at( id, 0 )) );
        test( qlist_remove( id, 0 ) );
        // list: 
        test( qlist_empty( id ) );
        test( 0 == qlist_len( id ) );
    }

    // append last
    {
        test( qlist_append_last( id, "x" ) );
        // list: x
        test( 'x' == (*(char*)qlist_at( id, 0 )) );
        test( NULL == qlist_at( id, 1 ) );
        test( !qlist_empty( id ) );
        test( 1 == qlist_len( id ) );
        test( qlist_remove( id, 0 ) );
        // list: 
        test( qlist_empty( id ) );
    }

    // append
    {
        test( qlist_append( id, 0, "y" ) );
        // list: 0 y
        test( !qlist_empty( id ) );
        test( 2 == qlist_len( id ) );
        test( qlist_remove( id, 0 ) );
        // list: y
        test( 'y' == (*(char*)qlist_at( id, 0 )) );
        test( qlist_remove( id, 0 ) );
        // list: 
        test( qlist_empty( id ) );
    }

    // insert
    {
        test( qlist_insert( id, 0, "z" ) );
        // list: z 0
        test( !qlist_empty( id ) );
        test( 2 == qlist_len( id ) );
        test( qlist_remove( id, 1 ) );
        // list: z
        test( 'z' == (*(char*)qlist_at( id, 0 )) );
        test( qlist_remove( id, 0 ) );
        // list:
        test( qlist_empty( id ) );
    }

    test( qlist_close( id ) );
    id = 0;
    return true;
}





static bool test_2_node(void){
    int id = qlist_open( sizeof( char ) );
    // list:

    // set
    {
        test( qlist_set( id, 0, "a" ) );
        // qlist: a
        test( 1 == qlist_len( id ) );
        test( 'a' == (*(char*)qlist_at( id, 0 )) );

        test( qlist_set( id, 0, "b" ) );
        // qlist: b
        test( 1 == qlist_len( id ) );
        test( 'b' == (*(char*)qlist_at( id, 0 )) );

        test( qlist_set( id, 1, "c" ) );
        // qlist: b c
        test( 2 == qlist_len( id ) );
        test( 'b' == (*(char*)qlist_at( id, 0 )) );
        test( 'c' == (*(char*)qlist_at( id, 1 )) );
        test( NULL == qlist_at( id, 2 ) );

        test( qlist_set( id, 1000, "d" ) );
        // qlist: b c 0 0 ... d(ix=1000)
        test( 1001 == qlist_len( id ) );
        test( 'b' == (*(char*)qlist_at( id, 0 )) );
        test( 'c' == (*(char*)qlist_at( id, 1 )) );
        test( 'd' == (*(char*)qlist_at( id, 1000 )) );

        test( qlist_remove_all( id ) );
        // list:
        test( qlist_empty( id ) );
        test( 0 == qlist_len( id ) );
    }

    // append last
    {
        test( qlist_append_last( id, "a" ) );
        // qlist: a
        test( 1 == qlist_len( id ) );
        test( 'a' == (*(char*)qlist_at( id, 0 )) );
        test( qlist_append_last( id, "b" ) );
        // qlist: a b
        test( 2 == qlist_len( id ) );
        test( 'a' == (*(char*)qlist_at( id, 0 )) );
        test( 'b' == (*(char*)qlist_at( id, 1 )) );

        test( qlist_remove_all( id ) );
        // list:
    }

    // insert
    {
        test( qlist_insert( id, 1, "a" ) );
        // list: 0 a 0
        test( 3 == qlist_len( id ) );
        test( 'a' == (*(char*)qlist_at( id, 1 )) );

        test( qlist_insert( id, 1, "b" ) );
        // list: 0 b a 0
        test( 4 == qlist_len( id ) );
        test( 'b' == (*(char*)qlist_at( id, 1 )) );
        test( 'a' == (*(char*)qlist_at( id, 2 )) );

        test( qlist_insert( id, 3, "c" ) );
        // list: 0 b a c 0
        test( 5 == qlist_len( id ) );
        test( 'b' == (*(char*)qlist_at( id, 1 )) );
        test( 'a' == (*(char*)qlist_at( id, 2 )) );
        test( 'c' == (*(char*)qlist_at( id, 3 )) );
    }

    test( qlist_close( id ) );
    id = 0;
    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_qlist(void){
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
