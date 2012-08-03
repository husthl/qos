
typedef struct qemdlink_node_s{
    //int id;                         // store qemdlink_id, simply interface function
    struct qemdlink_node_s* next;
    struct qemdlink_node_s* prev;
}qemdlink_node_t;
#define QEMDLINK_NODE_DEFAULT { NULL, NULL }
bool qemdlink_node_init( qemdlink_node_t* node );
bool qemdlink_ptr_init( int id, void* ptr );

typedef qemdlink_node_t* (*qemdlink_ptr_to_node_t)( void* ptr );
typedef void* (*qemdlink_node_to_ptr_t)( qemdlink_node_t* node );
qemdlink_node_t* qemdlink_node( int id, void* ptr );
void* qemdlink_ptr( int id, qemdlink_node_t* node );

bool qemdlink_id_valid( int id );
int qemdlink_open( qemdlink_ptr_to_node_t ptr_to_node, qemdlink_node_to_ptr_t node_to_ptr );
bool qemdlink_close( int id );


typedef bool (*qemdlink_for_eack_t)( int id, void* ptr, int ix, void* arg );
// return: for each node num
int qemdlink_for_each( int id, qemdlink_for_eack_t callback, void* arg );

//typedef bool (*qemdlink_node_printf_callback_t)( int id, void* ptr, int ix );
// return: printf node num
//int qemdlink_printf( int id, qemdlink_node_printf_callback_t callback );
int qemdlink_printf( int id, int ptr_size );

// information
int qemdlink_len( int id );
bool qemdlink_empty( int id );

// return: user ptr
void* qemdlink_first( int id );
void* qemdlink_last( int id );
void* qemdlink_next( int id, void* ptr );
void* qemdlink_prev( int id, void* ptr );

// set & add & remove
// you can link a const var & var on stack.
bool qemdlink_append_last( int id, void* src );
bool qemdlink_append( int id, void* here, void* src );
bool qemdlink_insert( int id, void* here, void* src );
bool qemdlink_remove( int id, void* ptr );

// index
void* qemdlink_at( int id, int ix );
bool qemdlink_in( int id, void* ptr );
int qemdlink_ix( int id, void* ptr );


// no more
