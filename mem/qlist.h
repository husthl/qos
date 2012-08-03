

int qlist_open( int node_len );
bool qlist_close( int id );
int qlist_dup( int id );

typedef bool (*qlist_for_eack_t)( void* node_p, int ix, void* arg );
int qlist_for_each( int id, qlist_for_eack_t callback, void* arg );

typedef bool (*qlist_node_printf_callback_t)( int ix, void* node_p );
// return: printf node num
int qlist_printf( int id, qlist_node_printf_callback_t callback );

// information
int qlist_len( int id );
bool qlist_empty( int id );

#if 0
// return: node_id
// 对qlist来说，无需使用node_id
int qlist_first( int id );
int qlist_last( int id );
int qlist_at( int id, int ix );
#endif

// set & add & remove
bool qlist_set( int id, int ix, const void* src );
bool qlist_append_last( int id, const void* src );
bool qlist_append( int id, int ix, const void* src );
bool qlist_insert( int id, int ix, const void* src );
bool qlist_remove( int id, int ix );
bool qlist_remove_all( int id );
bool qlist_mem_fit( int id );

const void* qlist_p( int id );
void* qlist_at( int id, int ix );


// no more
