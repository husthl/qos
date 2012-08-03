

#define DEFINE_FUN( f )     file_id_##f, 
typedef enum{
#include "file_list.h"
}file_id_t;
#undef DEFINE_FUN



#define this_file_name()    file_id_to_str( this_file_id )

const char* file_id_to_str( int file_id );
bool file_id_in_file_list( int file_id, int * const ix );

// no more
