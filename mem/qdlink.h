
int qdlink_open(void);
bool qdlink_close( int id );
int qdlink_dup( int id );

typedef bool (*qdlink_for_eack_t)( int node_id, int ix, void* arg );
int qdlink_for_each( int id, qdlink_for_eack_t callback, void* arg );

typedef bool (*qdlink_node_printf_callback_t)( int ix, int node_id );
// return: printf node num
int qdlink_printf( int id, qdlink_node_printf_callback_t callback );

// information
int qdlink_len( int id );
bool qdlink_empty( int id );

// return: node_id
int qdlink_first( int id );
int qdlink_last( int id );
int qdlink_next( int id, int node_id );
int qdlink_prev( int id, int node_id );
int qdlink_at( int id, int ix );

// set & add & remove
// node_src   must be alloc by qlink_malloc()
bool qdlink_append_last( int id, int node_src );
bool qdlink_append( int node_id, int node_src );
bool qdlink_insert( int id, int node_id, int node_src );
bool qdlink_delete( int id, int node_id );
int qdlink_delete_all( int id );

int qdlink_malloc( int size );
void* qdlink_p( int node_id );

// no more
