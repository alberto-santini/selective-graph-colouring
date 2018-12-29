#ifndef _OUTPUT_DBG_HPP
#define _OUTPUT_DBG_HPP

#ifdef NDEBUG
    #define DEBUG_ONLY(X) 
#else
    #define DEBUG_ONLY(X) X
#endif

#endif