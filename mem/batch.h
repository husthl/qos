
// arg          һ������malloc()��ȡ�Ŀռ�,����ȫ�ֱ���������static������
//              ��Ϊ������ʾ�ĵط����ͻ�ȡ��ʾ��Ϣ�ĵط�������ͬһ��������
//              ���ܴ����ջ�ϵı���
// return: ok?
typedef bool (*batch_callback_t)( void* arg );

bool batch_id_valid( int id );
int batch_open( int len );
bool batch_append( int id, batch_callback_t callback, void* arg );
bool batch_exec_close( int id );


//typedef bool (*stream_t)( const char* str );
//bool batch_to_str( int id, stream_t stream ){

// no more
