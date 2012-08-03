/*
 * File      : vfs.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 * Virtual File System
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-06     huanglin     first version
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>


#include "inc.h"
#include "qemdlink.h"
#include "qmem.h"
#include "batch.h"
#include "vfs.h"

#define this_file_id   file_id_vfs
 

//{{{ vfs link
typedef struct vfs_s{
    file_operation_t file_operation;

    qemdlink_node_t node;
}vfs_t;
static qemdlink_node_t* vfs_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    vfs_t* const p = (vfs_t*)ptr;
    return &(p->node);
}
static void* vfs_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, vfs_t, node );
    return p;
}
static int m_vfs_link = 0;
static bool vfs_link_open(void){
    if( 0 != m_vfs_link ){
        return true;
    }
    m_vfs_link = qemdlink_open( vfs_ptr_to_node, vfs_node_to_ptr );
    return_false_if( 0 == m_vfs_link );
    return true;
}
static bool vfs_link_close(void){
    uverify( 0 != m_vfs_link );
    return_false_if( !qemdlink_close( m_vfs_link ) );
    m_vfs_link = 0;
    return true;
}
static bool vfs_link_empty(void){
    return qemdlink_empty( m_vfs_link );
}



static bool file_operation_valid( const file_operation_t* file_operation ){
    if( NULL == file_operation ){
        return false;
    }
    return true;
}




static bool vfs_id_valid( int vfs_id ){
    if( 0 == vfs_id ){
        return false;
    }
    const vfs_t* const vfs = (vfs_t*)vfs_id;
    const file_operation_t* const op = &vfs->file_operation;
    if( NULL == op->path_match ){
        return false;
    }
    if( NULL == op->open ){
        return false;
    }
    if( NULL == op->close ){
        return false;
    }
    if( NULL == op->read ){
        return false;
    }
    if( NULL == op->write ){
        return false;
    }
    return true;
}





typedef struct{
    const file_operation_t* file_operation;
    int exist_vfs_id;
}vfs_exist_callback_t;
static bool vfs_exist_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_vfs_link == link_id );
    vfs_t* const vfs = (vfs_t*)ptr;
    uverify( NULL != vfs );
    uverify( 0 <= ix );
    vfs_exist_callback_t* const arg = (vfs_exist_callback_t*)void_arg;
    uverify( NULL != arg );

    const file_operation_t* const op = &vfs->file_operation;
    if( 0 == memcmp( op, arg->file_operation, sizeof( file_operation_t ) ) ){
        uverify( 0 == arg->exist_vfs_id );
        arg->exist_vfs_id = (int)vfs;
        // stop loop
        return false;
    }

    return true;
}
// return: the file_operation match vfs_id exist in vfs link.
static int vfs_exist_vfs_id( const file_operation_t* file_operation ){
    uverify( NULL != file_operation );
    return_false_if( !vfs_link_open() );

    vfs_exist_callback_t arg = { file_operation, 0 };
    qemdlink_for_each( m_vfs_link, vfs_exist_callback, &arg );
    return arg.exist_vfs_id;
}
static bool vfs_exist( const file_operation_t* file_operation ){
    uverify( NULL != file_operation );
    uverify( 0 != m_vfs_link );

    if( 0 == vfs_exist_vfs_id( file_operation ) ){
        return false;
    }
    return true;
}




// return: vfs_id
static int vfs_open( const file_operation_t* file_operation ){
    uverify( file_operation_valid( file_operation ) );

    return_false_if( !vfs_link_open() );
    // vfs open twice or more?
    return_false_if( vfs_exist( file_operation ) );

    vfs_t* const vfs = qmalloc( sizeof( vfs_t ) );
    return_0_if( NULL == vfs );

    if( GCFG_DEBUG_EN ){
        memset( vfs, 0, sizeof( vfs_t ) );
    }
    return_0_if( !qemdlink_node_init( &vfs->node ) );
    vfs->file_operation = *file_operation;

    return_0_if( !qemdlink_append_last( m_vfs_link, vfs ) );
    const int vfs_id = (int)vfs;
    uverify( vfs_id_valid( vfs_id ) );
    return vfs_id;
}
static bool vfs_close( int vfs_id ){
    uverify( vfs_id_valid( vfs_id ) );
    uverify( 0 != m_vfs_link );

    vfs_t* const vfs = (vfs_t*)vfs_id;
    return_false_if( !qemdlink_remove( m_vfs_link, vfs ) );
    if( GCFG_DEBUG_EN ){
        memset( vfs, 0, sizeof(vfs_t) );
    }
    qfree( vfs );

    if( vfs_link_empty() ){
        return_false_if( !vfs_link_close() );
    }
    return true;
}
//}}}



//{{{ file system
// for close all vfs id
static int m_batch = 0;

static bool batch_callback_close_vfs_id( void* arg ){
    const int vfs_id = (int)arg;
    uverify( 0 != vfs_id );
    return_false_if( !vfs_close( vfs_id ) );
    return true;
}
static bool valid_file_operation( const file_operation_t* fop ){
    if( NULL == fop ){
        return false;
    }

    const file_operation_t* const p = fop;
    if( NULL == p->path_match ){
        return false;
    }
    if( NULL == p->open ){
        return false;
    }
    if( NULL == p->close ){
        return false;
    }
    if( NULL == p->read ){
        return false;
    }
    if( NULL == p->write ){
        return false;
    }
    if( NULL == p->fstat ){
        return false;
    }
    return true;
}
// return: ok?
bool file_system_register_vfs( const file_operation_t* fop ){
    uverify( valid_file_operation( fop ) );
    uverify( 0 != m_batch );

    const int vfs_id = vfs_exist_vfs_id( fop );
    if( 0 != vfs_id ){
        return true;
    }

    const int vfs_id_new = vfs_open( fop );
    return_false_if( 0 == vfs_id_new );
    return_false_if( !batch_append( m_batch, batch_callback_close_vfs_id, (void*)vfs_id_new ) );
    return true;
}




bool file_system_init(void){
    uverify( 0 == m_batch );
    m_batch = batch_open( 10 );
    return_false_if( 0 == m_batch );
    
    return true;
}





bool file_system_exit(void){
    uverify( 0 != m_batch );
    return_false_if( !batch_exec_close( m_batch ) );
    return true;
}
//}}}




//{{{ file operation
static fid_base_t* fid_to_base( int fid ){
    uverify( 0 != fid );
    fid_base_t* const base = (fid_base_t*)fid;
    return base;
}



int fid_to_vfs_id( const fid_base_t* base ){
    uverify( NULL != base );
    return base->vfs_id;
}



typedef struct{
    const char* path;
    int vfs_id;
}qpath_match_callback_t;
static bool qpath_match_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_vfs_link == link_id );
    vfs_t* const vfs = (vfs_t*)ptr;
    uverify( NULL != vfs );
    uverify( 0 <= ix );
    qpath_match_callback_t* const arg = (qpath_match_callback_t*)void_arg;
    uverify( NULL != arg );

    const file_operation_t* const op = &vfs->file_operation;
    const bool match = op->path_match( arg->path );
    if( match ){
        uverify( 0 == arg->vfs_id );
        arg->vfs_id = (int)vfs;
        // stop loop
        return false;
    }
    return true;
}
// path_match loop through vfs link, if any vfs node path_match() return valid vfs_id,
// it will return.
// return: vfs_id that can handle this path.
int qpath_match( const char* path ){
    uverify( NULL != path );
    uverify( 0 != m_vfs_link );

    qpath_match_callback_t arg = { path, 0 };
    qemdlink_for_each( m_vfs_link, qpath_match_callback, &arg );
    return arg.vfs_id;
}



/*
typedef struct{
    const char* path;
    int oflag;
    void* config;
    int fid;
}qopen_callback_t;
static bool qopen_callback( int link_id, void* ptr, int ix, void* void_arg ){
    uverify( m_vfs_link == link_id );
    vfs_t* const vfs = (vfs_t*)ptr;
    uverify( NULL != vfs );
    uverify( 0 <= ix );
    qopen_callback_t* const arg = (qopen_callback_t*)void_arg;
    uverify( NULL != arg );

    const file_operation_t* const op = &vfs->file_operation;
    arg->fid = op->open( arg->path, arg->oflag, arg->config );
    if( 0 != arg->fid ){
        fid_base_t* const base = fid_to_base( arg->fid );
        base->vfs_id = (int)vfs;
        return false;
    }
    return true;
}
// open loop through vfs link, if any vfs node open() return valid fid, 
// it will return.
// return: fid use in read(), write(),   0, fail. 
int qopen( const char* path, int oflag, void* config ){
    uverify( NULL != path );
    uverify( 0 != m_vfs_link );

    qopen_callback_t arg = { path, oflag, config, 0 };
    qemdlink_for_each( m_vfs_link, qopen_callback, &arg );
    return arg.fid;
}
*/
static int vfs_qopen( int vfs_id, const char* path, int oflag, void* config ){
    uverify( vfs_id_valid( vfs_id ) );
    uverify( NULL != path );

    vfs_t* const vfs = (vfs_t*)vfs_id;
    const file_operation_t* const op = &vfs->file_operation;
    return op->open( path, oflag, config );
}
// return: fid use in read(), write(),   0, fail. 
int qopen( const char* path, int oflag, void* config ){
    uverify( NULL != path );
    uverify( 0 != m_vfs_link );
    
    const int vfs_id = qpath_match( path );
    // if vfs_id = 0, there's no valid vfs for this path.
    if( 0 == vfs_id ){
        return 0;
    }

    const int fid = vfs_qopen( vfs_id, path, oflag, config );
    fid_base_t* const base = fid_to_base( fid );
    base->vfs_id = vfs_id;
    return fid;
}





static int vfs_qclose( int vfs_id, int fid ){
    uverify( vfs_id_valid( vfs_id ) );
    uverify( 0 != fid );

    vfs_t* const vfs = (vfs_t*)vfs_id;
    const file_operation_t* const op = &vfs->file_operation;
    return op->close( fid );
}
bool qclose( int fid ){
    uverify( 0 != fid );

    fid_base_t* const base = fid_to_base( fid );
    const int vfs_id = base->vfs_id;
    uverify( vfs_id_valid( vfs_id ) );
    return vfs_qclose( vfs_id, fid );
}





static int vfs_qread( int vfs_id, int fid, void* buf, int nbyte ){
    uverify( vfs_id_valid( vfs_id ) );
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    vfs_t* const vfs = (vfs_t*)vfs_id;
    const file_operation_t* const op = &vfs->file_operation;
    return op->read( fid, buf, nbyte );
}
int qread( int fid, void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    fid_base_t* const base = fid_to_base( fid );
    const int vfs_id = base->vfs_id;
    uverify( vfs_id_valid( vfs_id ) );
    return vfs_qread( vfs_id, fid, buf, nbyte );
}



static int vfs_qwrite( int vfs_id, int fid, const void* buf, int nbyte ){
    uverify( vfs_id_valid( vfs_id ) );
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    vfs_t* const vfs = (vfs_t*)vfs_id;
    const file_operation_t* const op = &vfs->file_operation;
    return op->write( fid, buf, nbyte );
}
int qwrite( int fid, const void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    fid_base_t* const base = fid_to_base( fid );
    const int vfs_id = base->vfs_id;
    uverify( vfs_id_valid( vfs_id ) );
    return vfs_qwrite( vfs_id, fid, buf, nbyte );
}



static int vfs_qfstat( int vfs_id, int fid, struct qstat* buf ){
    uverify( vfs_id_valid( vfs_id ) );
    uverify( 0 != fid );
    uverify( NULL != buf );

    vfs_t* const vfs = (vfs_t*)vfs_id;
    const file_operation_t* const op = &vfs->file_operation;
    return op->fstat( fid, buf );
}
bool qfstat( int fid, struct qstat* buf ){
    uverify( 0 != fid );
    uverify( NULL != buf );

    fid_base_t* const base = fid_to_base( fid );
    const int vfs_id = base->vfs_id;
    uverify( vfs_id_valid( vfs_id ) );
    return vfs_qfstat( vfs_id, fid, buf );


}
//}}}




//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0

// file0: can store a char
typedef struct{
    fid_base_t base;
    char a;
}fid_0_t;
static bool path_match0( const char* path ){
    if( 0 == strcmp( "0", path ) ){
        return true;
    }
    return false;
}
static int open0( const char* path, int oflag, void* config ){
    uverify( NULL != path );
    uverify( NULL == config );
    if( 0 != strcmp( path, "0" ) ){
        return 0;
    }

    fid_0_t* const p = qmalloc( sizeof( fid_0_t ) );
    return_false_if( NULL == p );
    
    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_0_t ) );
    }

    return (int)p;
}

static bool close0( int fid ){
    uverify( 0 != fid );

    fid_0_t* const p = (fid_0_t*)fid;
    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_0_t ) );
    }
    qfree( p );
    return true;
}
static int read0( int fid, void *buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    if( 0 == nbyte ){
        return 0;
    }
    fid_0_t* const p = (fid_0_t*)fid;
    char* const dst = buf;
    *dst = p->a;
    return 1;
}
static int write0( int fid, const void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    if( 0 == nbyte ){
        return 0;
    }
    fid_0_t* const p = (fid_0_t*)fid;
    const char* const src = buf;
    p->a = *src;
    return 1;
}
static const file_operation_t m_file0 = { path_match0, open0, close0, read0, write0 };





// file1: when read, return char '1'-'9', then back to '1'. etc.
typedef struct{
    fid_base_t base;
    char start; 
}fid_1_t;
static bool path_match1( const char* path ){
    if( 0 == strcmp( "1", path ) ){
        return true;
    }
    return false;
}
static int open1( const char* path, int oflag, void* config ){
    uverify( NULL != path );
    uverify( NULL == config );
    if( 0 != strcmp( path, "1" ) ){
        return 0;
    }

    fid_1_t* const p = qmalloc( sizeof( fid_1_t ) );
    return_false_if( NULL == p );
    
    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_1_t ) );
    }

    p->start = '1';
    return (int)p;
}

static bool close1( int fid ){
    uverify( 0 != fid );

    fid_1_t* const p = (fid_1_t*)fid;
    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_1_t ) );
    }
    qfree( p );
    return true;
}
static int read1( int fid, void *buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    if( 0 == nbyte ){
        return 0;
    }
    fid_1_t* const p = (fid_1_t*)fid;
    char* const dst = buf;
    int i = 0;
    for( i=0; i<nbyte; i++ ){
        dst[i] = p->start;
        if( '9' == p->start ){
            p->start = '1';
        }
        else{
            p->start += 1;
        }
    }
    return nbyte;
}
// file1 not support write
static int write1( int fid, const void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    return 0;
}
static const file_operation_t m_file1 = { path_match1, open1, close1, read1, write1 };



static bool test_one_file(void){
    const int vfs0_id = vfs_open( &m_file0 );
    test( 0 != vfs0_id );

    test( 0 == qopen( "1", 0, NULL ) );

    {
        const int file0_id = qopen( "0", 0, NULL );
        test( 0 != file0_id );
        test( qclose( file0_id ) );
    }

    const int file0_id = qopen( "0", 0, NULL );
    char buf[10];
    test( 1 == qwrite( file0_id, "12345", 5 ) );
    test( 1 == qread( file0_id, buf, 10 ) );
    test( '1' == buf[0] );

    // read again
    test( 1 == qread( file0_id, buf, 10 ) );
    test( '1' == buf[0] );

    // write other char
    test( 1 == qwrite( file0_id, "xyz", 3 ) );
    test( 1 == qread( file0_id, buf, 10 ) );
    test( 'x' == buf[0] );

    test( qclose( file0_id ) );
    test( vfs_close( vfs0_id ) );

    // no vfs exist
    //test( 0 == qopen( "0", 0, NULL ) );
    return true;
}





static bool test_two_file(void){
    const int vfs0_id = vfs_open( &m_file0 );
    test( 0 != vfs0_id );
    const int vfs1_id = vfs_open( &m_file1 );
    test( 0 != vfs1_id );

    const int file0_id = qopen( "0", 0, NULL );
    const int file1_id = qopen( "1", 0, NULL );

    // file0 operation
    {
        char buf[10];
        test( 1 == qwrite( file0_id, "12345", 5 ) );
        test( 1 == qread( file0_id, buf, 10 ) );
        test( '1' == buf[0] );
    }

    // file1 operation
    {
        char buf[5];
        // file1 not support write
        test( 0 == qwrite( file1_id, "567", 3 ) );
        test( 5 == qread( file1_id, buf, 5 ) );
        test( 0 == memcmp( buf, "12345", 5 ) );

        char buf1[6];
        test( 6 == qread( file1_id, buf1, 6 ) );
        //printf( "\n%.*s\n", buf1 );
        test( 0 == memcmp( buf1, "678912", 6 ) );
    }


    test( qclose( file0_id ) );
    test( qclose( file1_id ) );

    test( vfs_close( vfs0_id ) );
    test( vfs_close( vfs1_id ) );
    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_vfs(void){
    const unit_test_item_t table[] = {
        test_one_file
        , test_two_file
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}




// no more------------------------------
