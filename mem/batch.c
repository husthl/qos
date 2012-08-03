/*
 * File      : batch.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-04     huanglin     first version
 *
 */



/*
 * batch���ƣ���Ϊ�˽���������⣺һ��Ҫ��һ�����̽�����
 * ��Ҫ��һЩ��������Ȼ������ʾ����������
 *      ����
 *      ����
 *      ��ʾ
 *  ����ʾ����Ϣ������ʾ��λ�ã�����һ���ط�������ӿڣ�����Ϊ�˽��
 *  ������⡣
 *  ���Գ���Ϊ���ӳ�ִ��һЩ��Ϊ�����������м䣬�������һЩ��Ϊ��
 *  ����������ͳһִ�С�
 *
 *  �÷���
 *  1   �ӳ���ʾ��Ϣ
 *  2   һ����ʾ�����Ϣ
 *  3   ��Ϣ����ʾ���ݣ������̾���
 *
 */


// ��׼��
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "inc.h"
#include "qmem.h"
#include "batch.h"

#define this_file_id   file_id_batch



static bool batch_callback_nothing( void * const arg ){
    uverify( NULL == arg );
    return true;
}
typedef struct{
    batch_callback_t callback;
    void* arg;
}callback_arg_t;
#define CALLBACK_ARG_DEFAULT {                      \
    batch_callback_nothing, NULL                    \
}


typedef struct{
    callback_arg_t* list;
    int len;                // ������� batch ����

    int num;                // ʵ�ʴ洢�˶��ٸ���Чbatch, ��num<=len
}batch_t;







static bool init_list( callback_arg_t* list, int len ){
    uverify( NULL != list );
    uverify( 0 <= len );

    const callback_arg_t the_default = CALLBACK_ARG_DEFAULT;

    int i = 0;
    for( i=0; i<len; i++ ){
        list[i] = the_default;
    }

    return true;
}






static bool callback_arg_valid( batch_callback_t callback, void* arg ){
    if( NULL == callback ){
        return false;
    }

    if( callback == batch_callback_nothing ){
        if( arg != NULL ){
            return false;
        }
    }

    return true;
}





static bool callback_arg_t_valid( const callback_arg_t* ca ){
    if( NULL == ca ){
        return false;
    }

    if( !callback_arg_valid( ca->callback, ca->arg ) ){
        return false;
    }

    return true;
}





static bool callback_arg_is_default( const callback_arg_t* callback_arg ){
    uverify( NULL != callback_arg );

    const callback_arg_t the_default = CALLBACK_ARG_DEFAULT;
    if( 0 != memcmp( callback_arg, &the_default, sizeof( callback_arg_t ) ) ){
        return false;
    }

    return true;
}





// ��len - num ����ȱʡֵ
static bool callback_arg_list_valid( const callback_arg_t* callback_arg_list, int len, int num ){
    if( NULL == callback_arg_list ){
        return false;
    }

    int i = 0;
    for( i=0; i<len; i++ ){
        callback_arg_t const * const p = &(callback_arg_list[i]);
        if( !callback_arg_t_valid( p ) ){
            return false;
        }

        if( i >= num ){
            if( !callback_arg_is_default( p ) ){
                return false;
            }
        }
    }

    return true;
}






static bool id_valid( int id ){
    batch_t const * const p = (batch_t*)id;
    if( NULL == p ){
        return false;
    }

    if( 0 > p->len ){
        return false;
    }

    if( 0 > p->num ){
        return false;
    }

    if( p->num > p->len ){
        return false;
    }

    if( !callback_arg_list_valid( p->list, p->len, p->num ) ){
        return false;
    }

    return true;
}
bool batch_id_valid( int id ){
    return id_valid( id );
    
}



/*
static bool print_callback_arg_t( int ix, const callback_arg_t* callback_arg, stream_t stream ){
    uverify( NULL != callback_arg );
    uverify( NULL != stream );

    const char* const msg = sprintf_on_mem( alloca, "%02d: callback<0x%08x> arg<0x%08x>"END_LINE
            , ix, (int)callback_arg->callback, (int)callback_arg->arg );
    return_false_if( !stream( msg ) );
    return true;
}
bool batch_to_str( int id, stream_t stream ){
    uverify( 0 != id );
    uverify( NULL != stream );

    {
        const char* const msg = sprintf_on_mem( alloca, "Batch: <0x%08x>"END_LINE, id );
        return_false_if( !stream( msg ) );
    }

    batch_t* const p = (batch_t*)id;

    {
        const int object = (int)(&p->object);
        uverify( object == id );
        return_false_if( !object_to_str_default( object, stream ) );
    }
    
    {
        const char* const msg = sprintf_on_mem( alloca, "\tlen: %d"END_LINE, p->len );
        return_false_if( !stream( msg ) );
    }
    {
        const char* const msg = sprintf_on_mem( alloca, "\tnum: %d"END_LINE, p->num );
        return_false_if( !stream( msg ) );
    }

    {
        int i = 0;
        for( i=0; i<p->len; i++ ){
            return_false_if( !print_callback_arg_t( i, &(p->list[i]), stream ) );
        }
    }

    return true;
}
*/



// len: it's startup len, when batch list is full ,it will be auto growed up.
// return: batch_id
int batch_open( int len ){
    uverify( 0 < len );

    callback_arg_t* const list = qmalloc( len * sizeof( callback_arg_t ) );
    return_0_if( NULL == list );
    return_0_if( !init_list( list, len ) );

    batch_t* const batch = qmalloc( sizeof( batch_t ) );
    if( NULL == batch ){
        qfree( list );
        uverify(  false );
        return 0;
    }

    batch->list = list;
    batch->len = len;
    batch->num = 0;

    const int id = (int)batch;
    uverify( id_valid( id ) );
    return id;
}





static bool callback_arg_set( callback_arg_t* callback_arg, batch_callback_t callback, void* arg ){
    uverify( callback_arg_t_valid( callback_arg ) );
    uverify( callback_arg_valid( callback, arg ) );

    callback_arg->callback = callback;
    callback_arg->arg = arg;

    return true;
}




static bool callback_arg_set_default( callback_arg_t* callback_arg ){
	uverify( callback_arg_t_valid( callback_arg ) );
    return_false_if( !callback_arg_set( callback_arg, batch_callback_nothing, NULL ) );
    return true;
}





static bool callback_arg_exec( callback_arg_t* callback_arg ){
    uverify( callback_arg_t_valid( callback_arg ) );
    callback_arg_t* const p = callback_arg;
    const batch_callback_t callback = p->callback;
    void* const arg = p->arg;

    //log_str( "batch callback_arg_exec() run callback:<0x%08x> arg:<0x%08x>"END_LINE, callback, arg );
    return_false_if( !callback( arg ) );

    return_false_if( !callback_arg_set_default( p ) );
    uverify( callback_arg_t_valid( p ) );
    return true;
}




static bool batch_full( int id ){
    uverify( id_valid( id ) );
    batch_t* const batch = (batch_t*)id;

    if( batch->num < batch->len ){
        return false;
    }

    return true;
}



static bool batch_auto_grow_up( int id ){
    uverify( id_valid( id ) );

    batch_t* const batch = (batch_t*)id;

    const int len = batch->len;
    const int increase_len = 50;
    const int new_len = len + increase_len;

    callback_arg_t* const new_list = qrealloc( batch->list, new_len * sizeof( callback_arg_t ) );
    // ���û���㹻�Ŀռ䣬�Ͽռ�û���ͷţ���Ȼ���ڡ��򵥷���false��˵��grow_upʧ�ܡ�
    return_false_if( NULL == new_list );
    batch->list = new_list;
    // �Ͽռ����ݣ��Ѿ�������ǰ�棬ֻ�ó�ʼ������һ�μ��ɡ�
    return_false_if( !init_list( batch->list + len, increase_len ) );

    batch->len = new_len;

    uverify( id_valid( id ) );
    return true;
}



// arg      ����callback�еĲ�����ԭ�򣺸ñ���ֻ���Ǿ�̬������
//          ������malloc����Ķ�����������callback��free����close
//          ������Ϊ�����callback��appendʱ���ǳ��ѿ������˳��
//          ���磺
//          const int some_id = some_id_open();
//          batch_append( id, batch_callback_close_some_id, (void*)some_id );
//          batch_append( id, batch_callback_display_some_id, (void*)some_id );
//          �����˳�򣬾ͻᵼ�µڶ���callback����ʱ��ʹ�����Ѿ���close��some_id��
//          ����һ����������ǣ�����һ��batch_insert()��������batch_callback_display_some_id
//          ����batch_callback_close_some_idǰ�棬�����������������
//          batchģ������Ѷȣ�Ҳ�����˵���ʱ���Ѷȡ����arg֮��������ϵ�Ƚϸ��ӣ�
//          batch_insert()�ͻ��кܶ������ġ����׷��֡����Ե����⡣
bool batch_append( int id, batch_callback_t callback, void* arg ){
    uverify( id_valid( id ) );
    uverify( callback_arg_valid( callback, arg ) );

    batch_t* const batch = (batch_t*)id;

    // �б����ˣ�
    if( batch_full( id ) ){
        if( !batch_auto_grow_up( id ) ){
            uverify( false );
            return false;
        }
    }
    uverify( !batch_full( id ) );

    callback_arg_t* const free_ix = &(batch->list[batch->num]);
    uverify( callback_arg_is_default( free_ix ) );
    if( !callback_arg_set( free_ix, callback, arg ) ){
        uverify( false );
        return false;
    }
    batch->num++;
    uverify( id_valid( id ) );
    
    return true;
}




static bool batch_close( int id ){
    uverify( id_valid( id ) );

    batch_t* p = (batch_t*)id;
    qfreez( p->list );
    qfreez( p );
    return true;
}




// �� batch �е����лص�����������ִ��
// �����һ������false��ֻuverify()��ʾ����Ȼִ�к����callback
// return: all callback return true?
//          false, error & any callback return false
bool batch_exec_close( int id ){
    uverify( id_valid( id ) );
    batch_t* const batch = (batch_t*)id;

    bool all_ok = true;
    int i = 0;
    for( i=0; i<batch->num; i++ ){
        callback_arg_t* const p = &(batch->list[i]);
        if( !callback_arg_exec( p ) ){
            all_ok = false;
            uverify( false );
        }
    }

    return_false_if( !batch_close( id ) );
    return all_ok;
}










//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static int m_cnt = 0;




static bool callback_0( void * const arg ){
    uverify( NULL != arg );
    const int n = *((int*)arg);
    qfree( arg );

    if( n != 0 ){
        return false;
    }

    m_cnt++;
    return true;
}




static bool callback_1( void * const arg ){
    uverify( NULL != arg );
    const int n = *((int*)arg);
    qfree( arg );

    if( n != 1 ){
        return false;
    }

    m_cnt++;
    return true;
}




static bool test_all(void){
    int id = batch_open( 3 );
    test( 0 != id );

    int* const n0 = qmalloc( sizeof( int ) );
    test( NULL != n0 );
    *n0 = 0;
    test( batch_append( id, callback_0, n0 ) );

    int* const n1 = qmalloc( sizeof( int ) );
    test( NULL != n1 );
    *n1 = 1;
    test( batch_append( id, callback_1, n1 ) );

    test( batch_append( id, batch_callback_nothing, NULL ) );
    // full
    // can success because list auto increase.
    test( batch_append( id, batch_callback_nothing, NULL ) );

    m_cnt = 0;
    test( batch_exec_close( id ) );
	id = 0;
    test( 2 == m_cnt );

    return true;
}





// unit test entry, add entry in file_list.h
extern bool unit_test_batch(void);
bool unit_test_batch(void){
    const unit_test_item_t table[] = {
        test_all
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}



// no more------------------------------
