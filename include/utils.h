#ifndef 	UTILS_H_
#define 	UTILS_H_

#include <sys/mman.h>
#include <stdint.h>

#define 	PROT_FLAGS 		(PROT_READ | PROT_WRITE)
#define 	MAP_FLAGS 		(MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE)
#define 	INTER_PROC_SEM		1
#define 	INTRA_PROC_SEM 		0


/* Syntactic sugar for compiler barrier */
static inline void barrier()
{
        asm volatile("":::"memory");
}

/* Syntactic sugar for atomic fetch and increment */
static inline uint64_t fetch_and_increment(volatile uint64_t *var)
{
        long counter_value = 1;
        asm volatile ("lock; xaddq %%rax, %1;"
                      : "=a" (counter_value), "+m" (*var)
                      : "a" (counter_value)
                      : "memory");
        return counter_value + 1;
} 


#endif 		// UTILS_H_
