#include "vector.h"

VECTOR_DEFINE_INIT(u8vec, u8, u8vec, data)
VECTOR_DEFINE_WRITE(u8vec, u8, u8vec, data)
VECTOR_DEFINE_FREE(u8vec, u8, u8vec, data)
VECTOR_DEFINE_SEARCH(u8vec, u8, u8vec, data)
VECTOR_DEFINE_DELETE(u8vec, u8, u8vec, data)

VECTOR_DEFINE_INIT(vec_size_t, size_t, vec_size_t, data)
VECTOR_DEFINE_WRITE(vec_size_t, size_t, vec_size_t, data)
VECTOR_DEFINE_FREE(vec_size_t, size_t, vec_size_t, data)
