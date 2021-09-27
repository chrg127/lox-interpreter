#include "vector.h"

#include "memory.h"

VECTOR_DEFINE_INIT(  VecU8, u8, vec_u8, data)
VECTOR_DEFINE_WRITE( VecU8, u8, vec_u8, data)
VECTOR_DEFINE_FREE(  VecU8, u8, vec_u8, data)
VECTOR_DEFINE_SEARCH(VecU8, u8, vec_u8, data)
VECTOR_DEFINE_DELETE(VecU8, u8, vec_u8, data)

VECTOR_DEFINE_INIT( VecSizet, size_t, vec_size_t, data)
VECTOR_DEFINE_WRITE(VecSizet, size_t, vec_size_t, data)
VECTOR_DEFINE_FREE( VecSizet, size_t, vec_size_t, data)

VECTOR_DEFINE_INIT( VecDouble, double, vec_double, data)
VECTOR_DEFINE_WRITE(VecDouble, double, vec_double, data)
VECTOR_DEFINE_FREE( VecDouble, double, vec_double, data)
