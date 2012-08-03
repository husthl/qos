/*
 * File      : linux_file.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-31     huanglin     first version
 *
 */



#include <stdio.h>  
#include <stdlib.h> 
#include <string.h>
#include <fcntl.h>      /*文件控制定义*/
#include <errno.h>      /*错误号定义*/
#include <unistd.h>
#include <termios.h>    /*PPSIX终端控制定义*/
#include <sys/stat.h>

#include "inc.h"
#include "vfs.h"
#include "serial.h"
#include "qmem.h"

#define this_file_id   file_id_linux_file


typedef struct{
    fid_base_t base;
    int linux_file_id;
}fid_t;


// the file must be open in the directory of the exe
static bool mpath_match( const char* path ){
    const char* const serial_path_pattern = "./";
    const int nbyte = strlen( serial_path_pattern );
    if( 0 == memcmp( serial_path_pattern, path, nbyte ) ){
        return true;
    }
    return false;
}
static int mopen( const char* path, int oflag, void* config ){
    uverify( NULL != path );

    const int linux_file_id = open( path, oflag );
    if( -1 == linux_file_id ){
        return 0;
    }

    fid_t* const p = qmalloc( sizeof( fid_t ) );
    return_false_if( NULL == p );

    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_t ) );
    }
    p->linux_file_id = linux_file_id;
    return (int)p;
}

static bool mclose( int fid ){
    uverify( 0 != fid );

    fid_t* const p = (fid_t*)fid;
    return_false_if( 0 != close( p->linux_file_id ) );
    if( GCFG_DEBUG_EN ){
        memset( p, 0, sizeof( fid_t ) );
    }
    qfree( p );
    return true;
}
static int mread( int fid, void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    fid_t* const p = (fid_t*)fid;
    return read( p->linux_file_id, buf, nbyte );
}
static int mwrite( int fid, const void* buf, int nbyte ){
    uverify( 0 != fid );
    uverify( NULL != buf );
    uverify( 0 <= nbyte );

    fid_t* const p = (fid_t*)fid;
    const int nwrite = write( p->linux_file_id, buf, nbyte );
    if( 0 > nwrite ){
        printf( "error: %s\n", strerror( errno ) );
        uverify( false );
    }
    return nwrite;
}
static bool mfstat( int fid, struct qstat* buf ){
    uverify( 0 != fid );
    uverify( NULL != buf );

    fid_t* const p = (fid_t*)fid;

    struct stat s;
    if( 0 != fstat( p->linux_file_id, &s) ){
        return false;
    }
    buf->size = s.st_size;
    return true;
}


bool linux_file_vfs_open(void){
    const file_operation_t fo = { mpath_match, mopen , mclose , mread , mwrite, mfstat };
    return_false_if( 0 == file_system_register_vfs( &fo ) );
    return true;
}






//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0



static bool test_all(void){
    test( linux_file_vfs_open() );

    // write
    int fid = qopen( "./a.txt", O_RDWR | O_CREAT | O_TRUNC, NULL );
    test( 0 != fid );
    test( 5 == qwrite( fid, "12345", 5 ) );
    {
        struct qstat buf;
        test( qfstat( fid, &buf ) );
        test( 5 == buf.size );
    }
    test( qclose( fid ) );
    fid = 0;

    // read
    fid = qopen( "./a.txt", O_RDONLY, NULL );
    test( 0 != fid );
    {
        struct qstat buf;
        test( qfstat( fid, &buf ) );
        test( 5 == buf.size );
    }
    char buf[100];
    memset( buf, 0, sizeof( buf ) );
    test( 5 == qread( fid, buf, sizeof(buf) ) );
    test( 0 == memcmp( buf, "12345", 5 ) );
    test( qclose( fid ) );
    fid = 0;

    return true;
}


// unit test entry, add entry in file_list.h
bool unit_test_linux_file(void){
    // 本文件单元测试项表格
    const unit_test_item_t table[] = {
		test_all
        , NULL              
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}


