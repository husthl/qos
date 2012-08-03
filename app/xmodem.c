/*
 * File      : xmodem.c
 * This file is part of qos
 * COPYRIGHT (C) 2012 - 2012, huanglin. All Rights Reserved
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-07-26     huanglin     first version
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "inc.h"
#include "vfs.h"
#include "qmem.h"
#include "qlist.h"
#include "xmodem.h"

#define this_file_id   file_id_xmodem
 
/*
The Xmodem Protocol Specification 

MODEM PROTOCOL OVERVIEW

1/1/82 by Ward Christensen. I will maintain a master copy of
this. Please pass on changes or suggestions via CBBS/Chicago
at (312) 545-8086, or by voice at (312) 849-6279.

NOTE this does not include things which I am not familiar with,
such as the CRC option implemented by John Mahr.

Last Rev: (none)

At the request of Rick Mallinak on behalf of the guys at
Standard Oil with IBM P.C.s, as well as several previous
requests, I finally decided to put my modem protocol into
writing. It had been previously formally published only in the
AMRAD newsletter.

Table of Contents
1. DEFINITIONS
2. TRANSMISSION MEDIUM LEVEL PROTOCOL
3. MESSAGE BLOCK LEVEL PROTOCOL
4. FILE LEVEL PROTOCOL
5. DATA FLOW EXAMPLE INCLUDING ERROR RECOVERY
6. PROGRAMMING TIPS.

-------- 1. DEFINITIONS.
<soh> 01H
<eot> 04H
<ack> 05H
<nak> 15H
<can> 18H

-------- 2. TRANSMISSION MEDIUM LEVEL PROTOCOL
Asynchronous, 8 data bits, no parity, one stop bit.

The protocol imposes no restrictions on the contents of the
data being transmitted. No control characters are looked for
in the 128-byte data messages. Absolutely any kind of data may
be sent - binary, ASCII, etc. The protocol has not formally
been adopted to a 7-bit environment for the transmission of
ASCII-only (or unpacked-hex) data , although it could be simply
by having both ends agree to AND the protocol-dependent data
with 7F hex before validating it. I specifically am referring
to the checksum, and the block numbers and their ones-
complement.
Those wishing to maintain compatibility of the CP/M file
structure, i.e. to allow modemming ASCII files to or from CP/M
systems should follow this data format:
* ASCII tabs used (09H); tabs set every 8.
* Lines terminated by CR/LF (0DH 0AH)
* End-of-file indicated by ^Z, 1AH. (one or more)
* Data is variable length, i.e. should be considered a
continuous stream of data bytes, broken into 128-byte
chunks purely for the purpose of transmission. 
* A CP/M "peculiarity": If the data ends exactly on a
128-byte boundary, i.e. CR in 127, and LF in 128, a
subsequent sector containing the ^Z EOF character(s)
is optional, but is preferred. Some utilities or
user programs still do not handle EOF without ^Zs.
* The last block sent is no different from others, i.e.
there is no "short block". 

-------- 3. MESSAGE BLOCK LEVEL PROTOCOL
Each block of the transfer looks like:
<SOH><blk #><255-blk #><--128 data bytes--><cksum>
in which:
<SOH> = 01 hex
<blk #> = binary number, starts at 01 increments by 1, and
wraps 0FFH to 00H (not to 01)
<255-blk #> = blk # after going thru 8080 "CMA" instr, i.e.
each bit complemented in the 8-bit block number.
Formally, this is the "ones complement".
<cksum> = the sum of the data bytes only. Toss any carry.

-------- 4. FILE LEVEL PROTOCOL

---- 4A. COMMON TO BOTH SENDER AND RECEIVER:

All errors are retried 10 times. For versions running with
an operator (i.e. NOT with XMODEM), a message is typed after 10
errors asking the operator whether to "retry or quit".
Some versions of the protocol use <can>, ASCII ^X, to
cancel transmission. This was never adopted as a standard, as
having a single "abort" character makes the transmission
susceptible to false termination due to an <ack> <nak> or <soh>
being corrupted into a <can> and canceling transmission.
The protocol may be considered "receiver driven", that is,
the sender need not automatically re-transmit, although it does
in the current implementations.

---- 4B. RECEIVE PROGRAM CONSIDERATIONS:
The receiver has a 10-second timeout. It sends a <nak>
every time it times out. The receiver's first timeout, which
sends a <nak>, signals the transmitter to start. Optionally,
the receiver could send a <nak> immediately, in case the sender
was ready. This would save the initial 10 second timeout. 
However, the receiver MUST continue to timeout every 10 seconds
in case the sender wasn't ready.
Once into a receiving a block, the receiver goes into a
one-second timeout for each character and the checksum. If the
receiver wishes to <nak> a block for any reason (invalid
header, timeout receiving data), it must wait for the line to
clear. See "programming tips" for ideas
Synchronizing: If a valid block number is received, it
will be: 1) the expected one, in which case everything is fine;
or 2) a repeat of the previously received block. This should
be considered OK, and only indicates that the receivers <ack>
got glitched, and the sender re-transmitted; 3) any other block
number indicates a fatal loss of synchronization, such as the
rare case of the sender getting a line-glitch that looked like
an <ack>. Abort the transmission, sending a <can>

---- 4C. SENDING PROGRAM CONSIDERATIONS.

While waiting for transmission to begin, the sender has
only a single very long timeout, say one minute. In the
current protocol, the sender has a 10 second timeout before
retrying. I suggest NOT doing this, and letting the protocol
be completely receiver-driven. This will be compatible with
existing programs.
When the sender has no more data, it sends an <eot>, and
awaits an <ack>, resending the <eot> if it doesn't get one. 
Again, the protocol could be receiver-driven, with the sender
only having the high-level 1-minute timeout to abort.

-------- 5. DATA FLOW EXAMPLE INCLUDING ERROR RECOVERY

Here is a sample of the data flow, sending a 3-block message.
It includes the two most common line hits - a garbaged block,
and an <ack> reply getting garbaged. <xx> represents the
checksum byte.

SENDER RECEIVER
times out after 10 seconds,
                        <--- <nak>
<soh> 01 FE -data- <xx> --->
                        <--- <ack>
<soh> 02 FD -data- xx   ---> (data gets line hit)
                        <--- <nak>
<soh> 02 FD -data- xx   --->
                        <--- <ack>
<soh> 03 FC -data- xx   --->
    (ack gets garbaged) <--- <ack>
<soh> 03 FC -data- xx   ---> <ack>
<eot>                   --->
                        <--- <ack>

-------- 6. PROGRAMMING TIPS.

* The character-receive subroutine should be called with a
parameter specifying the number of seconds to wait. The
receiver should first call it with a time of 10, then <nak> and
try again, 10 times.
After receiving the <soh>, the receiver should call the
character receive subroutine with a 1-second timeout, for the
remainder of the message and the <cksum>. Since they are sent
as a continuous stream, timing out of this implies a serious
like glitch that caused, say, 127 characters to be seen instead
of 128.

* When the receiver wishes to <nak>, it should call a "PURGE"
subroutine, to wait for the line to clear. Recall the sender
tosses any characters in its UART buffer immediately upon
completing sending a block, to ensure no glitches were mis-
interpreted.
The most common technique is for "PURGE" to call the
character receive subroutine, specifying a 1-second timeout,
and looping back to PURGE until a timeout occurs. The <nak> is
then sent, ensuring the other end will see it.

* You may wish to add code recommended by Jonh Mahr to your
character receive routine - to set an error flag if the UART
shows framing error, or overrun. This will help catch a few
more glitches - the most common of which is a hit in the high
bits of the byte in two consecutive bytes. The <cksum> comes
out OK since counting in 1-byte produces the same result of
adding 80H + 80H as with adding 00H + 00H.
*/



/*
参考文章：
1   Xmodem function source code
    http://www.menie.org/georges/embedded/xmodem.html
2   用UART做文件传输(采用Xmodem协议)
3   研究Xmodem协议必看的11个问题


如何启动接收？
xmodem协议，传输启动，本来是发送端先启动，接收端发送"C"或者NAK启动传输过程。
qshell的xmodem，因为操作限制，实际是接收端先启动等待传输开始。
操作顺序为：
1   在命令行上输入“xmodem_rx”，这时，qshell xmodem已经启动，等待发送端启动
    传输。如果超时未启动传输，则传输终止。
2   在SecureCRT菜单上选择：传输->发送Xmodem->选择文件。
3   等待传输完成。
*/


#include "crc16.h"

#define SOH  0x01
#define STX  0x02
#define EOT  0x04
#define ACK  0x06
#define NAK  0x15
#define CAN  0x18
#define CTRLZ 0x1A

#define DLY_1S 1000
#define MAX_TRANSMIT_CNT        25



typedef struct{
    int serial_fid;
    int rx_task_id;

    xmodem_info_t info;

    xmodem_end_t end_callback;
    void* end_callback_arg;

    int receive_id;
    int transmit_id;
}xmodem_t;




//{{{ xmodem basic

static bool id_valid( int id ){
    if( 0 == id ){
        return false;
    }
    return true;
}




static void flushinput( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const xmodem = (xmodem_t*)id;

    char buf[512];
    while( 0 < qread( xmodem->serial_fid, buf, sizeof(buf) ) ){
        ;
    }
    return;
}
static bool m_write_printf_en = true;
static bool write_serial( int id, char ch ){
    uverify( id_valid( id ) );
    xmodem_t* const xmodem = (xmodem_t*)id;

    return_false_if( !qwrite( xmodem->serial_fid, &ch, 1 ) );
    if( m_write_printf_en ){
        printf( "serial write: 0x%02x\n", ch );
    }
    return true;
}
/*
static bool read_serial( int id, char* ch ){
    uverify( id_valid( id ) );
    xmodem_t* const xmodem = (xmodem_t*)id;
    uverify( NULL != ch );

    if( 1 == qread( xmodem->serial_fid, ch, 1 ) ){
        if( m_read_printf_en ){
            printf( "serial read: 0x%02x\n", *((unsigned char*)ch) );
        }
        return true;
    }
    return false;
}
*/


//}}}



//{{{ rx_task

/*
    本模块是以接收串口字符来驱动的，即接收一个字符，处理，等待下一个字符，处理，如此循环。要考虑设置超时。
    即按照如下过程进行：
1   设置接收处理回调函数，并设置超时时间。
2   启动接收任务。接收任务中，收到一个字符，或者超时，都调用对应的回调函数。

*/


// return: process next char? false, do NOT call the callback again.
typedef bool (*rx_do_t)( char ch, bool timeout, void* arg );
#define CONST_VALUE    0x12ab34cd
typedef struct{
    int const_value;
    int serial_fid;

    rx_do_t rx_do;
    void* arg;
    const char* rx_do_name;
    
    int task_id;

    int timeout_task_id;
    int timeout_ms;
    bool timeout;
}rx_task_t;



static bool rx_task_valid( const rx_task_t* rx_task ){
    if( NULL == rx_task ){
        return false;
    }

    const rx_task_t* const p = rx_task;
    if( CONST_VALUE != p->const_value ){
        uverify( false );
        return false;
    }

    return true;
}




static bool rx_timeout_timer( void* void_arg ){
    rx_task_t* const rx_task = void_arg;
    uverify( NULL != rx_task );

    uverify( !rx_task->timeout );
    rx_task->timeout = true;
    rx_task->timeout_task_id = 0;
    return false;
}
// return: the callback return.
static bool rx_task_rx_do( rx_task_t* rx_task, char ch, bool timeout ){
    uverify( rx_task_valid( rx_task ) );
    return rx_task->rx_do( ch, timeout, rx_task->arg );
}
static bool m_read_printf_en = true;
static bool rx_task_read_serial( int serial_fid, char* ch ){
    uverify( 0 != serial_fid );
    uverify( NULL != ch );

    if( 1 == qread( serial_fid, ch, 1 ) ){
        if( m_read_printf_en ){
            printf( "serial read: 0x%02x\n", *((unsigned char*)ch) );
        }
        return true;
    }
    return false;
}
static bool rx_task_entry( void* void_arg );
// timeout timer must exit before rx_task_entry exit
// return: task again?
static bool read_char_task_entry( void* void_arg ){
    rx_task_t* const rx_task = void_arg;
    uverify( rx_task_valid( rx_task ) );

    char ch = 0;
    if( rx_task_read_serial( rx_task->serial_fid, &ch ) ){
        uverify( 0 != rx_task->timeout_task_id );
        return_false_if( !qos_task_close( rx_task->timeout_task_id ) );
        rx_task->timeout_task_id = 0;

        if( !rx_task_rx_do( rx_task, ch, false ) ){
            // do NOT rx char again.
            rx_task->task_id = 0;
            return false;
        }

        // rx next char, reinstall timeout timer and rx_do callback
        return_false_if( !qos_task_set_entry( 0, rx_task_entry, rx_task ) );
        return true;
    }

    if( rx_task->timeout ){
        uverify( 0 == rx_task->timeout_task_id );
        
        if( !rx_task_rx_do( rx_task, 0, true ) ){
            // do NOT rx char again.
            rx_task->task_id = 0;
            return false;
        }

        // rx next char, reinstall timeout timer and rx_do callback
        return_false_if( !qos_task_set_entry( 0, rx_task_entry, rx_task ) );
        return true;
    }

    // wait a char or timeout
    return true;
}
/*
// do some clean job
// return: notify again?
static bool rx_task_notify( int task_id, int event, void* arg ){
    rx_task_t* const rx_task = arg;
    uverify( rx_task_valid( rx_task ) );

    if( QOS_TASK_EVNET_CLOSE == event ){
        if( 0 != rx_task->timeout_task_id ){
            return_false_if( !qos_task_close( rx_task->timeout_task_id ) );
            rx_task->timeout_task_id = 0;
        }

        return false;
    }

    return true;
}
*/
static bool rx_task_entry( void* void_arg ){
    rx_task_t* const rx_task = void_arg;
    uverify( rx_task_valid( rx_task ) );

    // start timeout_timer
    const int us = MS_TO_US( rx_task->timeout_ms );
    uverify( 0 < us );
    if( 0 != rx_task->timeout_task_id ){
        return_false_if( !qos_task_close( rx_task->timeout_task_id ) );
        rx_task->timeout_task_id = 0;
    }
    rx_task->timeout = false;
    rx_task->timeout_task_id = qos_timer_open( rx_timeout_timer, rx_task, us );
    return_false_if( 0 == rx_task->timeout_task_id );

    return_false_if( !qos_task_set_entry( 0, read_char_task_entry, rx_task ) );
 //   return_false_if( !qos_task_set_notify( 0, rx_task_notify, rx_task ) );
    return true;
}



#define rx_task_start( rx_task_id, rx_do, arg, timeout_ms )                                         \
    (bool)({                                                                                        \
            const bool _b = rx_task_start_real( rx_task_id, rx_do, arg, #rx_do, timeout_ms );       \
            _b;                                                                                     \
    })
// ro_do            when receive a char from serial, this callback will be called.
static bool rx_task_start_real( int rx_task_id, rx_do_t rx_do, void* arg, const char* rx_do_name, int timeout_ms ){
    uverify( 0 != rx_task_id );
    rx_task_t* const rx_task = (rx_task_t*)rx_task_id;
    uverify( NULL != rx_do );
    uverify( NULL != rx_do_name );
    uverify( 0 <= timeout_ms );

    rx_task_t* const p = rx_task;
    if( 0 != p->task_id ){
        return_false_if( !qos_task_close( p->task_id ) );
        p->task_id = 0;
    }

    if( NULL != p->rx_do_name ){
        qfreez( p->rx_do_name );
        uverify( NULL != p->rx_do );
    }

    const int size = strlen( rx_do_name ) + 1;
    char* const s = qmalloc( size );
    return_false_if( NULL == s );
    memcpy( s, rx_do_name, size );
    printf( "start_rx_task( %s )\n", rx_do_name );

    p->rx_do_name = s;
    p->rx_do = rx_do;
    p->timeout_ms = timeout_ms;
    p->arg = arg;

    uverify( 0 == p->task_id );
    return_false_if( 0 == ( p->task_id = qos_idle_open( rx_task_entry, rx_task ) ) );

    return true;
}
static int rx_task_open( int serial_fid ){
    uverify( 0 != serial_fid );

    const int size = sizeof( rx_task_t );
    rx_task_t* const p = qmalloc( size );
    return_0_if( NULL == p );

    memset( p, 0, size );
    p->const_value = CONST_VALUE;
    p->serial_fid = serial_fid;
    const int rx_task_id = (int)p;
    uverify( rx_task_valid( p ) );
    return rx_task_id;
}
bool rx_task_close( int rx_task_id ){
    rx_task_t* const p = (rx_task_t*)rx_task_id;
    uverify( rx_task_valid( p ) );

    if( 0 != p->timeout_task_id ){
        return_false_if( !qos_task_close( p->timeout_task_id ) );
        p->timeout_task_id = 0;
    }

    if( 0 != p->task_id ){
        return_false_if( !qos_task_close( p->task_id ) );
        p->task_id = 0;
    }
    if( NULL != p->rx_do_name ){
        qfree( (char*)p->rx_do_name );
        p->rx_do_name = NULL;
    }
    qfree( p );
    return true;
}



//}}}



//{{{ commond


// return: ok?
static bool sync_tx( int id ){
    uverify( id_valid( id ) );
    return_false_if( !write_serial( id, NAK ) );
    return true;
}


//}}}



//{{{ receive
typedef struct{
    int obj_file_fid;

    // sync
    int sync_cnt;

    // package
    int package_list_id;
    unsigned char package_no;
    int transmit_cnt;

    // result
    int rx_list_id;
}receive_t;
static int receive_open( int obj_file_fid ){
    uverify( 0 != obj_file_fid );

    const int size = sizeof( receive_t );
    receive_t* const p = qmalloc( size );
    memset( p, 0, size );
    return_0_if( NULL == p );

    p->obj_file_fid = obj_file_fid;
    p->sync_cnt = 0;
    p->package_no = 1;

    return_0_if( 0 == ( p->package_list_id = qlist_open( 1 ) ) );
    return_0_if( 0 == ( p->rx_list_id = qlist_open( 1 ) ) );
    return (int)p;
}
static bool receive_close( int receive_id ){
    uverify( 0 != receive_id );
    receive_t* const p = (receive_t*)receive_id;

    return_false_if( !qlist_close( p->package_list_id ) );
    p->package_list_id = 0;

    return_false_if( !qlist_close( p->rx_list_id ) );
    p->rx_list_id = 0;

    const int size = sizeof( receive_t );
    memset( p, 0, size );
    qfree( p );
    return true;
}




#define RX_END_NORMAL                  1
#define RX_END_CANCELED_BY_REMOTE      2
#define RX_END_SYNC_ERROR              3
#define RX_END_TOO_MANY_RETRY_ERROR    4
static bool receive_end( int id, int reason ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;

    x->info.rxing = false;
    uverify( 0 != x->receive_id ); 
    return_false_if( !receive_close( x->receive_id ) );
    x->receive_id = 0;

    return_false_if( !x->end_callback( x->end_callback_arg ) );
    printf( "xmodem rx end, reason=%d\n", reason  );
    return true;
}



static bool receive_start_sync_rx( int id );

static bool reject_tx( int id ){
    uverify( id_valid( id ) );
    flushinput( id );
    return_false_if( !write_serial( id, NAK ) );
    return true;
}
static bool printf_package_list( int package_list_id ){
    uverify( 0 != package_list_id );

    const int nbyte = qlist_len( package_list_id );
    int i = 0;
    printf( "rx_list: (%d bytes)\n", nbyte );
    for( i=0; i<nbyte; i++ ){
        printf( "0x%02x ", *(unsigned char*)qlist_at( package_list_id, i ) );
    }
    printf( "\n" );
    return true;

}
static bool check_package_128( int package_list_id, receive_t* receive ){
    uverify( 0 != package_list_id );
    uverify( NULL != receive );

    printf( "check package 128\n" );
    printf_package_list( package_list_id );
    {
        const int nbyte = qlist_len( package_list_id );
        if( 128 + 4 != nbyte ){
            uverify( false );
            return false;
        }
    }
    {
        const char* const p = qlist_at( package_list_id, 0 );
        if( *p != SOH ){
            uverify( false );
            return false;
        }
    }
    {
        const unsigned char* const package_no = qlist_at( package_list_id, 1 );
        const unsigned char* const package_no_xor = qlist_at( package_list_id, 2 );
        if( *package_no != (unsigned char)(~(*package_no_xor) ) ){
            uverify( false );
            return false;
        }

        if( receive->package_no != *package_no ){
            printf( "receive->package_no = %d  *pacakge_no=%d\n", receive->package_no, *package_no );
            uverify( false );
            return false;
        }

    }

    {
        int i = 0;
		unsigned char sum = 0;
		for( i=0; i<128; i++ ){
            const unsigned char* const p = qlist_at( package_list_id, 3+i );
			sum += *p;
		}
        const unsigned char* const sum_in_package = qlist_at( package_list_id, 3+128 );
		if( sum != *sum_in_package ){
            uverify( false );
            return false;
        }
    }
    
    return true;
}
static bool save_rx_result( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    receive_t* const r = (receive_t*)x->receive_id;

    int i = 0;
    for( i=0; i<128; i++ ){
        const char* const src = qlist_at( r->package_list_id, 3+i );
        return_false_if( !qlist_append_last( r->rx_list_id, src ) );
        x->info.rx_nbyte += 128;
    }
   
    // write to obj file fid
    {
        const char* const p = qlist_p( r->package_list_id );
        const char* const src = p + 3;
        uverify( 0 != r->obj_file_fid );
        return_false_if( 128 != qwrite( r->obj_file_fid, src, 128 ) );
    }
    return true;
}
static bool rx_retransmit( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    receive_t* const r = (receive_t*)x->receive_id;

    r->transmit_cnt++;
    if( r->transmit_cnt > MAX_TRANSMIT_CNT ){
        return false;
    }

    return true;
}
// return: use this callback receive next char?
static bool recv_package( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );
    receive_t* const r = (receive_t*)x->receive_id;

    if( timeout ){
        m_read_printf_en = true;
        return_false_if( !reject_tx( id ) );
        return_false_if( !receive_start_sync_rx( id ) );
        return false;
    }

    return_false_if( !qlist_append_last( r->package_list_id, &ch ) );
    //printf( "qlist_len = %d\n", qlist_len( x->package_list_id ) );

    if( 128 + 4 <= qlist_len( r->package_list_id ) ){
        m_read_printf_en = true;

        if( check_package_128( r->package_list_id, r ) ){
            r->package_no++;
            return_false_if( !save_rx_result( id ) );
            r->transmit_cnt = 0;

		    return_false_if( !write_serial( id, ACK ) );
            // next package
            return_false_if( !receive_start_sync_rx( id ) );
            return false;
        }

        // 重新传递
        if( !rx_retransmit( id ) ){
            flushinput( id );
		    return_false_if( !write_serial( id, CAN ) );
		    return_false_if( !write_serial( id, CAN ) );
		    return_false_if( !write_serial( id, CAN ) );
            /* too many retry error */
            return_false_if( !receive_end( id, RX_END_TOO_MANY_RETRY_ERROR  ) );
            return false;
        }

	    return_false_if( !write_serial( id, ACK ) );
        return_false_if( !receive_start_sync_rx( id ) );
        return false;
    }

    return true;
}
static bool start_recv( int id, char last_ch ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    receive_t* const r = (receive_t*)x->receive_id;

    printf( "start recv\n" );
    m_read_printf_en = false;
    
    return_false_if( !qlist_remove_all( r->package_list_id ) );
    return_false_if( !qlist_mem_fit( r->package_list_id ) );
    return_false_if( !qlist_append_last( r->package_list_id, &last_ch ) );

    return_false_if( !rx_task_start( x->rx_task_id, recv_package, x, 1 * 1000 ) );
    return true;
}




// return: this callback receive next char?
static bool rx_can_rx( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );

    if( timeout ){
	    flushinput( id );
		return_false_if( !write_serial( id, CAN ) );
		return_false_if( !write_serial( id, CAN ) );
		return_false_if( !write_serial( id, CAN ) );
		// sync error 
        return_false_if( !receive_end( id, RX_END_SYNC_ERROR ) );
        return false;
    }

    // 如果再收到一个CAN，认为接收被远端取消
    if( ch == CAN ){
        flushinput( id );
        return_false_if( !write_serial( id, ACK ) );
        receive_end( id, RX_END_CANCELED_BY_REMOTE ); /* canceled by remote */
        return false;
    }

    // ch != CAN
    flushinput( id );
    return_false_if( !write_serial( id, CAN ) );
    return_false_if( !write_serial( id, CAN ) );
    return_false_if( !write_serial( id, CAN ) );
    // sync error 
    return_false_if( !receive_end( id, RX_END_SYNC_ERROR ) );
    return false;
}



// return: ok?
// 启动接收阶段
static bool receive_sync_rx( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );
    receive_t* const r = (receive_t*)x->receive_id;

    if( timeout ){
        const int sec = x->info.sync_timeout_sec;
        printf( "xmodem sync timout %d second\n", r->sync_cnt * sec );
        r->sync_cnt++;
        if( r->sync_cnt > x->info.sync_timeout_cnt_max ){
	        flushinput( id );
		    write_serial( id, CAN );
		    write_serial( id, CAN );
		    write_serial( id, CAN );
		    // sync error
            
            receive_end( id, RX_END_SYNC_ERROR );
            return false;
        }

        return_false_if( !write_serial( id, NAK ) );
        return true;
    }

    // NOT timeout
    r->sync_cnt = 0;
    switch( ch ) {
        case SOH:
            return_false_if( !start_recv( id, SOH ) );
            return false;
        case STX:
            return_false_if( !start_recv( id, STX ) );
            return false;
        case EOT:
            flushinput( id );
            write_serial( id, ACK );

            /* normal end */
            x->info.ok = true;
            return_false_if( !receive_end( id, RX_END_NORMAL ) );
            return false;
        case CAN:
            return_false_if( !rx_task_start( x->rx_task_id, rx_can_rx, x, 1 * 1000 ) );
            return false;
        default:
            break;
    }

    flushinput( id );
    write_serial( id, CAN );
    write_serial( id, CAN );
    write_serial( id, CAN );
	receive_end( id, RX_END_SYNC_ERROR ); 
    /* sync error */
    return false;
}
static bool receive_start_sync_rx( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    uverify( 0 < x->info.sync_timeout_sec );
    const int timeout_ms = SEC_TO_MS( x->info.sync_timeout_sec );
    return_false_if( !rx_task_start( x->rx_task_id, receive_sync_rx, x, timeout_ms ) );
    return true;
}






bool xmodem_start_receive( int xmodem_id, int obj_file_fid ){
    uverify( 0 != xmodem_id );
    xmodem_t* const x = (xmodem_t*)xmodem_id;

    uverify( 0 == x->receive_id );
    return_false_if( 0 == ( x->receive_id = receive_open( obj_file_fid ) ) );

    x->info.rxing = true;
    x->info.rx_nbyte = 0;
    x->info.ok = false;

    return_false_if( !sync_tx( xmodem_id ) );
    return_false_if( !receive_start_sync_rx( xmodem_id ) );
    return true;
}


//}}}



//{{{ transmit
typedef struct{
    // sync
    int sync_cnt;
    int eot_cnt;
    bool crc;

    // package
    int package_list_id;
    unsigned char package_no;
    int transmit_cnt;

    // src_file
    int src_file_offset;
    int src_file_len;
    int src_file_fid;
}transmit_t;
int transmit_open( int src_file_fid ){
    const int size = sizeof( transmit_t );
    transmit_t* const p = qmalloc( size );
    memset( p, 0, size );
    return_0_if( NULL == p );

    p->sync_cnt = 0;
    p->package_no = 1;
    p->crc = false;

    p->src_file_fid = src_file_fid;
    struct qstat s;
    return_0_if( !qfstat( src_file_fid, &s ) );
    p->src_file_offset = 0;
    p->src_file_len = s.size;

    return_0_if( 0 == ( p->package_list_id = qlist_open( 1 ) ) );
    return (int)p;
}
bool transmit_close( int transmit_id ){
    uverify( 0 != transmit_id );
    transmit_t* const p = (transmit_t*)transmit_id;

    return_false_if( !qlist_close( p->package_list_id ) );
    p->package_list_id = 0;

    const int size = sizeof( transmit_t );
    memset( p, 0, size );
    qfree( p );
    return true;
}
static bool transmit_start_sync_rx( int id );
bool xmodem_start_transmit( int xmodem_id, int src_file_fid ){
    uverify( 0 != xmodem_id );
    uverify( 0 != src_file_fid );
    xmodem_t* const x = (xmodem_t*)xmodem_id;

    uverify( 0 == x->transmit_id );
    return_false_if( 0 == ( x->transmit_id = transmit_open( src_file_fid ) ) );

    x->info.txing = true;
    x->info.tx_nbyte = 0;
    x->info.ok = false;

    return_false_if( !sync_tx( xmodem_id ) );
    return_false_if( !transmit_start_sync_rx( xmodem_id ) );
    return true;
}




#define TX_END_NORMAL                   1
#define TX_END_NO_SYNC                  2
#define TX_END_CANCELED_BY_REMOTE       3
#define TX_END_XMIT_ERROR               4
#define TX_END_TOO_MANY_RETRY_ERROR     5
#define TX_END_EOT_NO_ACK               6
static bool transmit_end( int id, int reason ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    transmit_t* const t = (transmit_t*)x->transmit_id;

    x->info.txing = false;
    x->info.tx_nbyte = t->src_file_offset + 1;

    uverify( 0 != x->transmit_id ); 
    return_false_if( !transmit_close( x->transmit_id ) );
    x->transmit_id = 0;

    return_false_if( !x->end_callback( x->end_callback_arg ) );
    printf( "xmodem tx end, reason=%d\n", reason  );
    return true;
}




static bool tx_retransmit( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    transmit_t* const t = (transmit_t*)x->transmit_id;

    t->transmit_cnt++;
    if( t->transmit_cnt > MAX_TRANSMIT_CNT ){
        return false;
    }

    return true;
}
static bool tx_package( int xmodem_id, int package_list_id ){
    uverify( 0 != xmodem_id );
    uverify( 0 != package_list_id );

    const int len = qlist_len( package_list_id );
    uverify( len == 128 + 4 || len == 128 + 5 );
    int i = 0;
    m_write_printf_en = false;
    for( i=0; i<len; i++ ){
        const char* const p = qlist_at( package_list_id, i );
        return_false_if( !write_serial( xmodem_id, *p ) );
    }
    m_write_printf_en = true;

    return true;
}
// return: this callback receive next char?
static bool tx_can_rx( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );

    // 如果再收到一个CAN，认为接收被远端取消
    if( ch == CAN ){
        flushinput( id );
        return_false_if( !write_serial( id, ACK ) );
        transmit_end( id, TX_END_CANCELED_BY_REMOTE ); // canceled by remote 
        return false;
    }

    // timeout or ch!=CAN
    uverify( timeout || ch!=CAN );
    flushinput( id );
    return_false_if( !write_serial( id, CAN ) );
    return_false_if( !write_serial( id, CAN ) );
    return_false_if( !write_serial( id, CAN ) );
    // sync error 
    return_false_if( !transmit_end( id, TX_END_XMIT_ERROR ) );
    return false;
}
static bool start_tran( int id );
// return: use this callback receive next char?
static bool tran_package( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );
    transmit_t* const t = (transmit_t*)x->transmit_id;

    if( timeout ){
        if( !tx_retransmit( id ) ){
            flushinput( id );
		    return_false_if( !write_serial( id, CAN ) );
		    return_false_if( !write_serial( id, CAN ) );
		    return_false_if( !write_serial( id, CAN ) );
            // xmit error 
            return_false_if( !transmit_end( id, TX_END_TOO_MANY_RETRY_ERROR  ) );
            return false;
        }


        return_false_if( !tx_package( id, t->package_list_id ) );
        return true;
    }

    switch( ch ){
        case ACK:
            t->package_no++;
            t->src_file_offset += 128;
            return_false_if( !start_tran( id ) );
            return false;
        case CAN:
            return_false_if( !rx_task_start( x->rx_task_id, tx_can_rx, x, 1 * 1000 ) );
            return false;
        case NAK:
        default:
            return_false_if( !tx_package( id, t->package_list_id ) );
            return true;
            
    }

    return true;
}
// return: package nbyte into qlist
static int package_into_qlist( int package_list_id, unsigned char package_no, bool crc, int src_file_fid ){
    uverify( 0 != package_list_id );
    uverify( 0 != src_file_fid );

    int nbyte = 0;

    return_false_if( !qlist_remove_all( package_list_id ) );
    return_false_if( !qlist_mem_fit( package_list_id ) );

    {
        const char ch = SOH;
        return_false_if( !qlist_append_last( package_list_id, &ch ) );
    }
    
    {
        const unsigned char package_no_ch = package_no;
        const unsigned char package_no_ch_negate = ~package_no;
        return_false_if( !qlist_append_last( package_list_id, &package_no_ch ) );
        return_false_if( !qlist_append_last( package_list_id, &package_no_ch_negate ) );
    }
    {
        /*
        int i = 0;
        for( i=0; i<128; i++ ){
            const char ch = '0'+ ( i % 10 );
            return_false_if( !qlist_append_last( package_list_id, &ch ) );
        }
        */
        char buf[128];
        memset( buf, sizeof(buf), CTRLZ );
        nbyte = qread( src_file_fid, buf, 128 );
        int i = 0;
        for( i=0; i<128; i++ ){
            return_false_if( !qlist_append_last( package_list_id, buf+i ) );
        }

    }

    if( crc ){
        const char* const p = qlist_p( package_list_id );
	    unsigned short ccrc = crc16_ccitt( p + 3, 128 );
        const unsigned char hi = (ccrc>>8) & 0xFF;
        const unsigned char lo = ccrc & 0xFF;
        return_false_if( !qlist_append_last( package_list_id, &hi ) );
        return_false_if( !qlist_append_last( package_list_id, &lo ) );
    }
    else{
        const char* const p = qlist_p( package_list_id );
        int i = 0;
        unsigned char ccks = 0;
		for( i=3; i<128+3; ++i ){
			ccks += p[i];
		}
        return_false_if( !qlist_append_last( package_list_id, &ccks ) );
    }

    return true;
}
// return: use this callback receive next char?
static bool tx_eot( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );
    transmit_t* const t = (transmit_t*)x->transmit_id;

    if( ch == ACK ){
        x->info.ok = true;
        return_false_if( !transmit_end( id, TX_END_NORMAL ) );
        return false;
    }

    // timeout or ch!=ACK
    uverify( timeout || ch != ACK );
    t->eot_cnt++;
    if( t->eot_cnt > MAX_TRANSMIT_CNT ){
        flushinput( id );
        return_false_if( !transmit_end( id, TX_END_EOT_NO_ACK  ) );
        return false;
    }
	return_false_if( !write_serial( id, EOT ) );
    return true;
}
static bool start_tran( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    transmit_t* const t = (transmit_t*)x->transmit_id;

    printf( "start tran\n" );
    if( t->src_file_offset >= t->src_file_len ){
        t->eot_cnt = 0;
		return_false_if( !write_serial( id, EOT ) );
        return_false_if( !rx_task_start( x->rx_task_id, tx_eot, x, 1 * 1000 ) );
        return true;
    }
    //m_read_printf_en = false;
    return_false_if( !package_into_qlist( t->package_list_id, t->package_no, t->crc, t->src_file_fid ) );
    return_false_if( !tx_package( id, t->package_list_id ) );
    
    return_false_if( !rx_task_start( x->rx_task_id, tran_package, x, 1 * 1000 ) );
    return true;
}







// return: ok?
// 启动接收阶段
static bool transmit_sync_rx( char ch, bool timeout, void* arg ){
    xmodem_t* const x = (xmodem_t*)arg;
    const int id = (int)x;
    uverify( id_valid( id ) );
    transmit_t* const t = (transmit_t*)x->transmit_id;

    if( timeout ){
        const int sec = x->info.sync_timeout_sec;
        printf( "xmodem tx sync timout %d second\n", t->sync_cnt * sec );
        t->sync_cnt++;
        if( t->sync_cnt > x->info.sync_timeout_cnt_max ){
	        flushinput( id );
		    write_serial( id, CAN );
		    write_serial( id, CAN );
		    write_serial( id, CAN );
		    // no sync
            
            transmit_end( id, TX_END_NO_SYNC );
            return false;
        }

        return true;
    }

    // NOT timeout
    t->sync_cnt = 0;
    switch( ch ) {
        case 'C':
            t->crc = true;
            return_false_if( !start_tran( id ) );
            return false;
        case NAK:
            t->crc = false;
            return_false_if( !start_tran( id ) );
            return false;
        case CAN:
            return_false_if( !rx_task_start( x->rx_task_id, tx_can_rx, x, 1 * 1000 ) );
            return false;
        default:
            break;
    }

    flushinput( id );
    write_serial( id, CAN );
    write_serial( id, CAN );
    write_serial( id, CAN );
	transmit_end( id, TX_END_NO_SYNC ); 
    // no sync
    return false;
}
static bool transmit_start_sync_rx( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    uverify( 0 < x->info.sync_timeout_sec );
    const int timeout_ms = SEC_TO_MS( x->info.sync_timeout_sec );
    return_false_if( !rx_task_start( x->rx_task_id, transmit_sync_rx, x, timeout_ms ) );
    return true;
}

//}}}



//{{{ xmodem_t







bool xmodem_info( int id, xmodem_info_t* info ){
    uverify( id_valid( id ) );
    uverify( NULL != info );
    xmodem_t* const x = (xmodem_t*)id;
    *info = x->info;
    return true;
}
#define SYNC_TIMEOUT_SEC_DEFAULT            3
#define SYNC_TIMEOUT_CNT_MAX_DEFAULT        20       // 20 * 3 = 60 second
// set sync can change waiting transmit start time.
// transmit_start_time = sync_timeout_cnt_max * sync_timeout_sec (second)
bool xmodem_sync_set( int id, int sync_timeout_cnt_max, int sync_timeout_sec ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;

    x->info.sync_timeout_cnt_max = sync_timeout_cnt_max;
    x->info.sync_timeout_sec = sync_timeout_sec;
    return true;
}



static bool end_callback_default( void* arg ){
    uverify( NULL == arg );
    return true;
}
bool xmodem_set_end_callback( int id, xmodem_end_t end_callback, void* arg ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;
    uverify( NULL != end_callback );

    x->end_callback = end_callback;
    x->end_callback_arg = arg;
    return true;
}





// serial_fid       the serial file id
// obj_file_fid     the object file id
// rx               true, receive file; false, transmit file
int xmodem_open( int serial_fid ){
    uverify( 0 != serial_fid );

    xmodem_t* const x = qmalloc( sizeof( xmodem_t ) );
    return_0_if( NULL == x );
    memset( x, 0, sizeof( xmodem_t ) );

    x->serial_fid = serial_fid;

    x->info.ok = false;
    x->info.rxing = false;
    x->info.txing = false;

    return_0_if( 0 == ( x->rx_task_id = rx_task_open( serial_fid ) ) );

    const int id = (int)x;
    uverify( id_valid( id ) );
    return_0_if( !xmodem_sync_set( id, SYNC_TIMEOUT_CNT_MAX_DEFAULT, SYNC_TIMEOUT_SEC_DEFAULT ) );
    return_0_if( !xmodem_set_end_callback( id, end_callback_default, NULL ) );
    return id;
}
bool xmodem_close( int id ){
    uverify( id_valid( id ) );
    xmodem_t* const x = (xmodem_t*)id;

    return_false_if( !rx_task_close( x->rx_task_id ) );
    x->rx_task_id = 0;

    if( 0 != x->receive_id ){
        return_false_if( !receive_close( x->receive_id ) );
        x->receive_id = 0;
    }

    if( 0 != x->transmit_id ){
        return_false_if( !transmit_close( x->transmit_id ) );
        x->transmit_id = 0;
    }

    memset( x, 0, sizeof( xmodem_t ) );
    qfree( x );
    return true;
}



//}}}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static bool test_nothing(void){
    return true;
}


// unit test entry, add entry in file_list.h
bool unit_test_xmodem(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}




// no more------------------------------
