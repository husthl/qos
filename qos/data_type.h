

typedef int             bool;
#define true            1
#define false           0


#ifndef NULL
#define NULL            ((void*)0)
#endif

static inline bool bool_valid( int b ){ return b == true || b == false; }

// no more------------------------------
