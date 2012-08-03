
int qlink_open(void);
bool qlink_close( int id );

typedef bool (*qlink_for_eack_t)( int node_id, int ix, void* arg );
int qlink_for_each( int id, qlink_for_eack_t callback, void* arg );

typedef bool (*qlink_node_printf_callback_t)( int ix, int node_id );
// return: printf node num
int qlink_printf( int id, qlink_node_printf_callback_t callback );

// information
bool qlink_empty( int id );
int qlink_len( int id );

// return: node_id
int qlink_first( int id );
int qlink_last( int id );
int qlink_next( int node_id );
int qlink_prev( int id, int node_id );
int qlink_at( int id, int ix );

// set & add & remove
// node_src   must be alloc by qlink_malloc()
bool qlink_append_last( int id, int node_src );
bool qlink_append( int node_id, int node_src );
bool qlink_insert( int id, int node_id, int node_src );
bool qlink_delete( int id, int node_id );
int qlink_delete_all( int id );

int qlink_malloc( int size );
void* qlink_p( int node_id );

// no more
