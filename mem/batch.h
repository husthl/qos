
// arg          一定是用malloc()获取的空间,或者全局变量，或者static变量。
//              因为调用显示的地方，和获取显示信息的地方，不是同一个函数。
//              不能传入堆栈上的变量
// return: ok?
typedef bool (*batch_callback_t)( void* arg );

bool batch_id_valid( int id );
int batch_open( int len );
bool batch_append( int id, batch_callback_t callback, void* arg );
bool batch_exec_close( int id );


//typedef bool (*stream_t)( const char* str );
//bool batch_to_str( int id, stream_t stream ){

// no more
