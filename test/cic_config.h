
#include <stdint.h>

typedef int32_t cic_t;

#define CIC_MALLOC      1

#if CIC_MALLOC
#include <stdlib.h>
#define cic_malloc(x) malloc(x)
#define cic_free(x) free(x)
#endif
