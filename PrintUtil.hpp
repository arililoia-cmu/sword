#ifndef PRINTUTIL_HPP
#define PRINTUTIL_HPP

#include <iostream>

//#define ENABLE_DEBUG_PRINT

#ifdef ENABLE_DEBUG_PRINT
#define DEBUGOUT std::cerr
#else
#define DEBUGOUT 0 && std::cerr
#endif

#endif
