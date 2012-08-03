

int qshell_open( int fid );
bool qshell_close( int shell_id );
bool qshell_connect_fid( int shell_id );
bool qshell_disconnect_fid( int shell_id );


// command_line     it's user input after the shell prompt, the whole line include command.
//                  such as: ls -al
// return: shell again? false, exit shell
typedef bool (*cmd_entry_t)( int shell_id, const char* command_line, void* arg );
// return: cmd_id
int qshell_cmd_append( int shell_id, cmd_entry_t entry, const char* command, const char* help, void* arg );



// no more

