/*
 * File      : main.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-06     huanglin     first version
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>      /*文件控制定义*/

#include "inc.h"
#include "serial.h"
#include "qshell.h"
#include "vfs.h"
#include "qstr.h"
#include "xmodem.h"
#include "linux_file.h"


#define this_file_id   file_id_main


static int m_linux_fid = 0;
static int m_xmodem_id = 0;
static int m_serial_fid = 0;


/*
如果输入按键无效，检查键盘上Scroll灯是否亮着，如果亮了，按一下Scroll Lock.

 */

// return: shell again?
static bool cmd_entry_exit_qos( int shell_id, const char* command_line, void* arg ){
    uverify( 0 != shell_id );
    uverify( NULL != command_line );

    uverify( 0 != m_serial_fid );
    const char* const p = sprintf_on_mem( alloca, "\r\nqos exit( at qos tick %d )\r\n", qos_tick_cnt() );
    qwrite( m_serial_fid, p, strlen( p ) );

    return_false_if( !qshell_close( shell_id ) );

    return_false_if( !qclose( m_serial_fid ) );
    m_serial_fid = 0;

    uverify( 0 != m_xmodem_id );
    return_false_if( !xmodem_close( m_xmodem_id ) );
    m_xmodem_id = 0;

    return_false_if( !qos_exit() );
    return false;
}





static bool xmodem_tx_end_callback( void* arg ){
    const int shell_id = (int)arg;
    uverify( 0 != shell_id );

    uverify( 0 != m_linux_fid );
    return_false_if( !qclose( m_linux_fid ) );
    m_linux_fid = 0;

    xmodem_info_t info;
    uverify( 0 != m_xmodem_id );
    return_false_if( !xmodem_info( m_xmodem_id, &info ) );
    if( info.ok ){
        uverify( 0 != info.tx_nbyte );
        const int nbyte = info.tx_nbyte;
        const char* const m = sprintf_on_mem( alloca, "\rxmodem transmit OK, total %d bytes.\r\n", nbyte );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }
    else{
        const char* const m = sprintf_on_mem( alloca, "%s", "\rxmodem transmit FAIL\r\n" );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }

    // xmdoem协议结束，qshell接管串口读、写
    return_false_if( !qshell_connect_fid( shell_id ) );
    return true;
}

// return: shell again?
static bool cmd_entry_xmodem_tx( int id, const char* command_line, void* arg ){
    uverify( 0 != id );
    uverify( NULL != command_line );

    {
        const char* const m = sprintf_on_mem( alloca, "%s", "begin transmit file by by protocol xmodem\r\n" );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }

    // 因为xmodem协议、qshell同时从串口上读取数据，会造成混乱,
    // 所以，在xmdoem协议结束前，xmodem协议接管串口读、写
    return_false_if( !qshell_disconnect_fid( id ) );
    uverify( 0 != m_xmodem_id );

    {
        xmodem_info_t info;
        return_false_if( !xmodem_info( m_xmodem_id, &info ) );
        const int start_timeout_second = info.sync_timeout_cnt_max * info.sync_timeout_sec;
        const char* const m = sprintf_on_mem( alloca, "please start remote xmodem receive in %d second.\r\n", start_timeout_second );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }

    uverify( 0 == m_linux_fid );
    m_linux_fid = qopen( "./a.txt", O_RDONLY, NULL );
    return_false_if( 0 == m_linux_fid );

    return_false_if( !xmodem_set_end_callback( m_xmodem_id, xmodem_tx_end_callback, (void*)id ) );
    return_false_if( !xmodem_start_transmit( m_xmodem_id, m_linux_fid ) );
    return true;
}



static bool xmodem_rx_end_callback( void* arg ){
    const int shell_id = (int)arg;
    uverify( 0 != shell_id );

    uverify( 0 != m_linux_fid );
    return_false_if( !qclose( m_linux_fid ) );
    m_linux_fid = 0;

    xmodem_info_t info;
    uverify( 0 != m_xmodem_id );
    return_false_if( !xmodem_info( m_xmodem_id, &info ) );
    if( info.ok ){
        uverify( 0 != info.rx_nbyte );
        const int nbyte = info.rx_nbyte;
        const char* const m = sprintf_on_mem( alloca, "\rxmodem receive OK, total %d bytes.\r\n", nbyte );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }
    else{
        const char* const m = sprintf_on_mem( alloca, "%s", "\rxmodem receive FAIL\r\n" );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }

    // xmdoem协议结束，qshell接管串口读、写
    return_false_if( !qshell_connect_fid( shell_id ) );
    return true;
}

// return: shell again?
static bool cmd_entry_xmodem_rx( int id, const char* command_line, void* arg ){
    uverify( 0 != id );
    uverify( NULL != command_line );
    uverify( 0 != m_serial_fid );

    {
        const char* const m = sprintf_on_mem( alloca, "%s", "begin receive file by by protocol xmodem\r\n" );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }

    // 因为xmodem协议、qshell同时从串口上读取数据，会造成混乱
    // 所以，在xmdoem协议结束前，xmodem协议接管串口读、写
    return_false_if( !qshell_disconnect_fid( id ) );

    uverify( 0 != m_xmodem_id );

    {
        xmodem_info_t info;
        return_false_if( !xmodem_info( m_xmodem_id, &info ) );
        const int start_timeout_second = info.sync_timeout_cnt_max * info.sync_timeout_sec;
        const char* const m = sprintf_on_mem( alloca, "please start xmodem transmit in %d second.\r\n", start_timeout_second );
        return_false_if( !qwrite( m_serial_fid, m, strlen( m ) ) );
    }

    uverify( 0 == m_linux_fid );
    m_linux_fid = qopen( "./a.txt", O_RDWR | O_CREAT | O_TRUNC, NULL );
    return_false_if( 0 == m_linux_fid );

    return_false_if( !xmodem_set_end_callback( m_xmodem_id, xmodem_rx_end_callback, (void*)id ) );
    return_false_if( !xmodem_start_receive( m_xmodem_id, m_linux_fid ) );
    return true;
}




static bool app_init(void){
    if( GCFG_UNIT_TEST_EN ){
        return true;
    }
    return_false_if( !linux_file_vfs_open() );
    return_false_if( !serial_vfs_open() );
    //const char* const path = "/dev/ttyS0";
    //serial_t config = { 0, 9600, 8, 1, 'n', 0, 0 };
    // use bluetooth
    const char* const path = "/dev/ttyS19";
    serial_t config = { 0, 115200, 8, 1, 'n', 0, 0 };
    m_serial_fid = qopen( path, 0, &config );
    return_false_if( 0 == m_serial_fid );

    const int shell_id = qshell_open( m_serial_fid );
    return_false_if( 0 == shell_id );

    uverify( 0 == m_xmodem_id );
    m_xmodem_id = xmodem_open( m_serial_fid );
    return_false_if( 0 == m_xmodem_id );

    return_false_if( 0 == qshell_cmd_append( shell_id, cmd_entry_exit_qos, "qqos",  "quit qos", NULL ) );
    //return_false_if( !qos_set_busy( qos_busy_default_warnning ) );

    return_0_if( 0 == qshell_cmd_append( shell_id, cmd_entry_xmodem_rx, "r",    "receive file a.txt by protocol xmodem", NULL ) );
    return_0_if( 0 == qshell_cmd_append( shell_id, cmd_entry_xmodem_tx, "t",    "transmit file by protocol xmodem", NULL ) );
    return true;
}


int main(void){
    qos_init();
    app_init();
    qos_start();
    return 0;
}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static bool test_nothing(void){
    return true;
}




// unit test entry, add entry in file_list.h
bool unit_test_main(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}

