/*
 * File      : qshell.c
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
#include <stdlib.h>
#include <string.h>


#include "inc.h"
#include "vfs.h"
#include "qshell.h"
#include "qmem.h"
#include "qstr.h"
#include "qlist.h"
#include "qemdlink.h"
#include "xmodem.h"

#define this_file_id   file_id_qshell
 

typedef struct{
    int fid;
    bool connect_fid;
    int input_list_id;
    int task_id;
    int cmd_link;
}shell_t;
static bool id_valid( int id );



typedef struct{
    cmd_entry_t entry;
    const char* command;
    const char* help;
    void* arg;

    qemdlink_node_t node;           
}cmd_t;
static bool cmd_id_valid( int cmd_id );
static bool shell_prompt( int id );



//{{{ cmd link
static qemdlink_node_t* cmd_ptr_to_node( void* ptr ){
    uverify( NULL != ptr );
    cmd_t* const p = (cmd_t*)ptr;
    return &(p->node);
}
static void* cmd_node_to_ptr( qemdlink_node_t* node ){
    uverify( NULL != node );
    void* const p = container_of( node, cmd_t, node );
    return p;
}
static int cmd_link_open(void){
    const int link = qemdlink_open( cmd_ptr_to_node, cmd_node_to_ptr );
    return_false_if( 0 == link );
    return link;
}


static bool cmd_id_valid( int cmd_id ){
    if( 0 == cmd_id ){
        return false;
    }
    const cmd_t* const cmd = (cmd_t*)cmd_id;
    if( NULL == cmd->entry ){
        return false;
    }
    if( NULL == cmd->command ){
        return false;
    }
    if( NULL == cmd->help ){
        return false;
    }
    
    return true;
}



// return: cmd_id
static int cmd_open( int cmd_link, cmd_entry_t entry, const char* command, const char* help, void* arg ){
    uverify( 0 != cmd_link );
    uverify( NULL != entry );
    uverify( NULL != command );
    uverify( NULL != help );

    cmd_t* const cmd = qmalloc( sizeof( cmd_t ) );
    return_0_if( NULL == cmd );
    if( GCFG_DEBUG_EN ){
        memset( cmd, 0, sizeof( cmd_t ) );
    }
    return_0_if( !qemdlink_node_init( &cmd->node ) );

    cmd->entry = entry;
    cmd->arg = arg;

    cmd->command = str_dup_on_mem( qmalloc, command );
    return_0_if( NULL == cmd->command );

    cmd->help = str_dup_on_mem( qmalloc, help );
    return_0_if( NULL == cmd->help );

    return_0_if( !qemdlink_append_last( cmd_link, cmd ) );

    const int cmd_id = (int)cmd;
    uverify( cmd_id_valid( cmd_id ) );
    return cmd_id;
}
static bool cmd_close( int cmd_link, int cmd_id ){
    uverify( 0 != cmd_link );
    uverify( cmd_id_valid( cmd_id ) );

    cmd_t* const cmd = (cmd_t*)cmd_id;
    return_false_if( !qemdlink_remove( cmd_link, cmd ) );

    qfreez( cmd->command );
    qfreez( cmd->help );

    if( GCFG_DEBUG_EN ){
        memset( cmd, 0, sizeof(cmd_t) );
    }
    qfree( cmd );
    return true;
}



static bool cmd_link_close( int cmd_link ){
    uverify( 0 != cmd_link );

    for(;;){
        cmd_t* const cmd = qemdlink_first( cmd_link );
        if( NULL == cmd ){
            return_false_if( !qemdlink_close( cmd_link ) );
            cmd_link = 0;
            return true;
        }

        const int cmd_id = (int)cmd;
        return_false_if( !cmd_close( cmd_link, cmd_id ) );
    }

    uverify( false );
    return false;
}



typedef struct{
    const char* const command;
    int result_cmd_id;
}command_to_cmd_arg_t;
static bool command_to_cmd_id_callback( int cmd_link, void* ptr, int ix, void* void_arg ){
    uverify( 0 != cmd_link );
    const cmd_t* const cmd = ptr;
    uverify( NULL != cmd );
    const int cmd_id = (int)cmd;
    uverify( cmd_id_valid( cmd_id ) );
    uverify( 0 <= ix );
    command_to_cmd_arg_t* const arg = (command_to_cmd_arg_t*)void_arg;
    uverify( NULL != arg );

    uverify( NULL != arg->command );
    if( 0 == memcmp( cmd->command, arg->command, strlen( cmd->command ) ) ){
        uverify( 0 == arg->result_cmd_id );
        arg->result_cmd_id = cmd_id;
        return false;
    }

    return true;
}
// return: cmd_id
static int command_to_cmd_id( int cmd_link, const char* command, int len ){
    uverify( 0 != cmd_link );
    uverify( NULL != command );
    uverify( 0 < len );

    command_to_cmd_arg_t arg = { command, 0 };
    qemdlink_for_each( cmd_link, command_to_cmd_id_callback, &arg );
    return arg.result_cmd_id;
}




typedef struct{
    int shell_id;
}cmd_link_printf_help_arg_t;
static bool write_port_str( int shell_id, const char* str );
static bool cmd_link_printf_help_callback( int cmd_link, void* ptr, int ix, void* void_arg ){
    uverify( 0 != cmd_link );
    const cmd_t* const cmd = ptr;
    uverify( NULL != cmd );
    const int cmd_id = (int)cmd;
    uverify( cmd_id_valid( cmd_id ) );
    uverify( 0 <= ix );
    cmd_link_printf_help_arg_t* const arg = (cmd_link_printf_help_arg_t*)void_arg;
    uverify( NULL != arg );

    const int shell_id = arg->shell_id;
    const char* const p = sprintf_on_mem( alloca, "%d\t%s\t%s\r\n", ix+1, cmd->command, cmd->help );
    return_false_if( !write_port_str( shell_id, p ) );
    return true;
}
// return: ok?
static bool cmd_link_printf_help( int shell_id ){
    uverify( id_valid( shell_id ) );
    const shell_t* const shell = (shell_t*)shell_id;

    return_false_if( !write_port_str( shell_id, "qshell help:\r\n" ) );
    return_false_if( !write_port_str( shell_id, "num\tcommand\thelp\r\n" ) );

    cmd_link_printf_help_arg_t arg = { shell_id };
    qemdlink_for_each( shell->cmd_link, cmd_link_printf_help_callback, &arg );
    return true;
}

//}}}



//{{{ cmd_entry

// return: shell again?
static bool cmd_entry_exit( int id, const char* command_line, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != command_line );

    const char* const p = sprintf_on_mem( alloca, "\r\nqshell exit( at qos tick %d )\r\n", qos_tick_cnt() );
    return_false_if( !write_port_str( id, p ) );
    return_false_if( !qshell_close( id ) );
    //return_false_if( !qshell_task_exit( id ) );
    return false;
}
// return: shell again?
static bool cmd_entry_help( int id, const char* command_line, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != command_line );

    return_false_if( !cmd_link_printf_help( id ) );
    return true;
}

// return: shell again?
static bool cmd_entry_not_found( int id, const char* command_line, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != command_line );

    const int len = strlen( command_line );
    const char* const p = sprintf_on_mem( alloca, "Not found command( total %d char ): %s\r\n", len, command_line );
    return_false_if( !write_port_str( id, p ) );
    return true;
}
// true == string_is_blank( "" )
// true == string_is_blank( "        " )
static bool string_is_blank( const char* string ){
    uverify( NULL != string );
    const char* p = string;
    while( ' ' == *p ){
        p++;
    }
    if( 0x00 != *p ){
        return false;
    }
    return true;
}
// return: shell again?
static bool cmd_entry_null( int id, const char* command_line, void* arg ){
    uverify( id_valid( id ) );
    uverify( NULL != command_line );
    // command_line is "" or all blank
    uverify( string_is_blank( command_line ) );
    return true;
}




// the command len, such as cmd_line = "    command arg1 arg2", 
// after exec below function, result pointer to "command"
// cmd_len = strlen( "command" );
// return: pointer to first segment and length
const char* segment_p( const char* command_line, int* len ){
    uverify( NULL != command_line );
    uverify( NULL != len );

    const char* start = command_line;
    while( *start == ' ' ){
        start++;
    }
    // start point to None-blank char.

    const char* const end = strchr( start, ' ' );
    // whole command_line is command
    if( NULL == end ){
        *len = strlen( start );
        return start;
    }

    *len = end - command_line;
    uverify( 0 < *len );
    return start;
}




// return: cmd_t
static const cmd_t* command_line_cmd( int cmd_link, const char* command_line ){
    uverify( 0 != cmd_link );
    uverify( NULL != command_line );
    static const cmd_t cmd_null = { cmd_entry_null, "", "", NULL, QEMDLINK_NODE_DEFAULT };
    static const cmd_t cmd_not_found = { cmd_entry_not_found, "", "", NULL, QEMDLINK_NODE_DEFAULT };

    if( 0 == strlen( command_line ) ){
        return &cmd_null;
    }

    int command_len = 0;
    const char* const command_p = segment_p( command_line, &command_len );
    uverify( NULL != command_p );
    uverify( 0 <= command_len );

    if( 0 >= command_len ){
        return &cmd_null;
    }

    const int cmd_id = command_to_cmd_id( cmd_link, command_p, command_len );
    if( 0 == cmd_id ){
        return &cmd_not_found;
    }

    const cmd_t* const cmd = (cmd_t*)cmd_id;
    return cmd;
}





//}}}



//{{{ shell

// id       shell_id
static bool id_valid( int id ){
    if( 0 == id ){
        return false;
    }

    const shell_t* const p = (shell_t*)id;
    if( 0 == p->fid ){
        return false;
    }
    if( 0 == p->input_list_id ){
        return false;
    }
    if( 0 == p->task_id ){
        return false;
    }
    if( 0 == p->cmd_link ){
        return false;
    }

    return true;
}
static bool qshell_task_entry( void* arg );
static bool cmd_entry_exit( int id, const char* command_line, void* arg );
static bool cmd_entry_help( int id, const char* command_line, void* arg );

static bool qshell_task_start( shell_t* shell ){
    uverify( NULL != shell );

    uverify( 0 == shell->task_id );
    shell->task_id = qos_idle_open( qshell_task_entry, shell );
    return_false_if( 0 == shell->task_id );
    return_false_if( !qos_task_set_name( shell->task_id, "qshell_task" ) );
    return true;
}
bool qshell_connect_fid( int shell_id ){
    uverify( id_valid( shell_id ) );
    shell_t* const shell = (shell_t*)shell_id;

    shell->connect_fid = true;
    return_false_if( !shell_prompt( shell_id ) );
    return true;
}
bool qshell_disconnect_fid( int shell_id ){
    uverify( id_valid( shell_id ) );
    shell_t* const shell = (shell_t*)shell_id;

    shell->connect_fid = false;
    return true;
}

// fid      is the serial port( or other char file ) file id, the qopen() return value.
//          shell will be establish on the fid
// return: shell_id
int qshell_open( int fid ){
    uverify( 0 != fid );

    shell_t* const shell = qmalloc( sizeof( shell_t ) );
    return_0_if( NULL == shell );
    shell->fid = fid;
    shell->connect_fid = true;

    shell->input_list_id = qlist_open( sizeof( char ) );
    return_0_if( 0 == shell->input_list_id );

    shell->cmd_link = cmd_link_open();
    return_0_if( 0 == shell->cmd_link );

    const int shell_id = (int)shell;
    shell->task_id = 0;
    return_0_if( !qshell_task_start( shell ) );

    return_0_if( 0 == qshell_cmd_append( shell_id, cmd_entry_help, "?",     "this help", NULL ) );
    return_0_if( 0 == qshell_cmd_append( shell_id, cmd_entry_help, "help",  "help [command]", NULL ) );
    return_0_if( 0 == qshell_cmd_append( shell_id, cmd_entry_exit, "qshell",  "quit qshell", NULL ) );

    uverify( id_valid( shell_id ) );
    return shell_id;
}
static bool qshell_close_id( int id ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;

    return_false_if( !qlist_close( shell->input_list_id ) );
    return_false_if( !cmd_link_close( shell->cmd_link ) );

    if( GCFG_DEBUG_EN ){
        memset( shell, 0, sizeof( shell_t ) );
    }
    qfree( shell );
    return true;
}
/*
// return: notify again?    false, stop notify; true, notify again.
static bool task_notify( int task_id, int event, void* arg ){
    if( event == (int)qos_task_close ){
        const int shell_id = (int)arg;
        uverify( id_valid( shell_id ) );
        return_false_if( !qshell_close( shell_id ) );
        return false;
    }
    return true;
}
*/
bool qshell_close( int id ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;
    uverify( 0 != shell->task_id );

//    return_false_if( !qos_task_set_notify( shell->task_id, task_notify, (void*)id ) );
    return_false_if( !qos_task_close( shell->task_id ) );

    return_false_if( !qshell_close_id( id ) );
    shell->task_id = 0;
    return true;
}


// return: cmd_id
int qshell_cmd_append( int shell_id, cmd_entry_t entry, const char* command, const char* help, void* arg ){
    uverify( id_valid( shell_id ) );
    uverify( NULL != entry );
    uverify( NULL != command );
    uverify( NULL != help );

    shell_t* const shell = (shell_t*)shell_id;
    return cmd_open( shell->cmd_link, entry, command, help, arg );
}


// return: ok?
static bool write_port_str( int id, const char* str ){
    uverify( id_valid( id ) );
    uverify( NULL != str );

    shell_t* const shell = (shell_t*)id;
    const int fid = shell->fid;
    uverify( 0 != fid );
    const int nbyte = strlen( str );
    return_false_if( nbyte != qwrite( fid, str, nbyte ) );
    return true;
}




static const char* user_input( int id ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;
    const int input_list_id = shell->input_list_id;
    uverify( 0 != input_list_id );
    const char* const cmd_str = qlist_p( input_list_id );
    return cmd_str;
}
static bool user_input_clear( int id ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;
    const int input_list_id = shell->input_list_id;
    return_false_if( !qlist_remove_all( input_list_id ) );
    return_false_if( !qlist_mem_fit( input_list_id ) );
    return true;
}
static int user_input_len( int id ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;
    const int input_list_id = shell->input_list_id;
    const int len = qlist_len( input_list_id );
    return len;
}
// return: ok?
static bool user_input_append_ch( int id, char ch ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;
    const int input_list_id = shell->input_list_id;
    return_false_if( !qlist_append_last( input_list_id, &ch ) );
    return true;
}
static bool user_input_remove_last( int id ){
    uverify( id_valid( id ) );
    shell_t* const shell = (shell_t*)id;
    const int input_list_id = shell->input_list_id;
    const int len = user_input_len( id );
    uverify( 0 < len );
    return_false_if( !qlist_remove( input_list_id, len-1 ) );
    return true;
}



static bool shell_prompt( int id ){
    uverify( id_valid( id ) );
    const char* const prompt = "\rshell> ";
    return_false_if( !write_port_str( id, prompt ) );
    return true;
}


//}}}



//{{{ task


// return: shell again?
static bool shell_exec_command_real( int id ){
    uverify( id_valid( id ) );

    return_false_if( !user_input_append_ch( id, '\x00' ) );
    const char* const command_line = user_input( id );
    uverify( strlen( command_line ) == ( user_input_len( id ) - 1 ) );

    const shell_t* const shell = (shell_t*)id;
    const cmd_t* cmd = command_line_cmd( shell->cmd_link, command_line );
    const cmd_entry_t entry = cmd->entry;
    void* const arg = cmd->arg;
    uverify( NULL != entry );
    // if any command process return false, do NOT use shell id any more because it's already free.
    if( !entry( id, command_line, arg ) ){
        return false;
    }

    return true;
}
// return: shell again?
static bool shell_exec_command( int id ){
    uverify( id_valid( id ) );

    const bool shell_again = shell_exec_command_real( id );
    if( shell_again ){
        return_false_if( !user_input_clear( id ) );
        return_false_if( !shell_prompt( id ) );
        return true;
    }
    return false;
}




// return: shell again?
static bool ch_enter( int id ){
    uverify( id_valid( id ) );

    return_false_if( !write_port_str( id, "\r\n" ) );
    return shell_exec_command( id );
}



// return: shell again?
static bool ch_backup( int id ){
    uverify( id_valid( id ) );
    if( 0 >= user_input_len( id ) ){
        return true;
    }

    return_false_if( !user_input_remove_last( id ) );
    return_false_if( !write_port_str( id, "\b \b" ) );
    return true;
}



// return: shell again?
static bool ch_default( int id, char ch ){
    uverify( id_valid( id ) );

    return_false_if( !user_input_append_ch( id, ch ) );
    const char output[2] = { ch, '\0' };
    return_false_if( !write_port_str( id, output ) );
    return true;
}




#define ENTER_KEY 0x0d
#define BACK_KEY 0x08
// return: shell again?
static bool shell_ch( int id, char ch ){
    uverify( id_valid( id ) );

    switch( ch ){
        case ENTER_KEY:                 // 回车键，需要执行指令
            return ch_enter( id );
            break;

        case BACK_KEY:                  // 退格键
            return ch_backup( id );
            break;

        default:                        // 一般按键，保存并回显
            return ch_default( id, ch );
            break;
    }
    
    // cannot handle this char
    return false;
}



// return: shell again?
static bool shell_input( int id, const char* input, int nbyte ){
    uverify( id_valid( id ) );
    uverify( NULL != input );
    uverify( 0 <= nbyte );

    if( 0 == nbyte ){
        return true;
    }

    int i = 0;
    for( i=0; i<nbyte; i++ ){
        if( !shell_ch( id, input[i] ) ){
            return false;
        }
    }

    return true;
}



// return: ok?
static bool shell_start( int id ){
    uverify( id_valid( id ) );
    const char* const welcome = sprintf_on_mem( alloca, 
            "\r\n"
            "\r\n"
            "This is qshell( compile on %s %s ), writen by huanglin( husthl@gmail.com )\r\n"
            , __DATE__, __TIME__ );
    return_false_if( !write_port_str( id, welcome ) );
    return_false_if( !shell_prompt( id ) );
    return true;
}

static int read_fid( int shell_id, char* buf, int nbyte ){
    uverify( id_valid( shell_id ) );
    shell_t* const shell = (shell_t*)shell_id;
    uverify( NULL != buf );
    uverify( 0 < nbyte );

    if( !shell->connect_fid ){
        return 0;
    }

    uverify( 0 != shell->fid );
    return qread( shell->fid, buf, nbyte );
}

// arg              shell_t pointer
// return: callback again?
static bool qshell_task_entry_read_command_line( void* arg ){
    shell_t* const shell = (shell_t*)arg;
    uverify( NULL != shell );
    const int shell_id = (int)shell;

    char buf[256];
    const int nread = read_fid( shell_id, buf, sizeof( buf ) );
    if( !shell_input( shell_id, buf, nread ) ){
        return false;
    }
    return true;
}
static bool qshell_task_entry( void* arg ){
    shell_t* const shell = (shell_t*)arg;
    uverify( NULL != shell );
    const int shell_id = (int)shell;

    return_false_if( !shell_start( shell_id ) );
    return_false_if( !qos_task_set_entry( 0, qshell_task_entry_read_command_line, arg ) );
    return true;
}


//}}}



//{{{ unit test
#if GCFG_UNIT_TEST_EN > 0


static bool test_nothing(void){
    return true;
}


// unit test entry, add entry in file_list.h
bool unit_test_qshell(void){
    const unit_test_item_t table[] = {
        test_nothing
        , NULL
    };

    return unit_test_file_pump_table( table );
}

#endif
//}}}




// no more------------------------------
