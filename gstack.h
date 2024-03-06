#ifndef GSTACK_H
#define GSTACK_H

/* To use, do this:
 *   #define GSTACK_IMPLEMENTATION
 * before you include this file in *one* C file to create the implementation.
 *
 * i.e. it should look like:
 *   #include ...
 *   #include ...
 *
 *   #define GSTACK_IMPLEMENTATION
 *   #include "gstack.h"
 *   ...
 *
 * To make all the functions have internal linkage, i.e. be private to the
 * source file, do this:
 *   #define GSTACK_STATIC
 * before including "gstack.h"
 *
 * i.e. it should look like:
 *   #define GSTACK_IMPLEMENTATION
 *   #define GSTACK_STATIC
 *   #include "gstack.h"
 *   ...
 *
 * You can define GSTACK_MALLOC, GSTACK_REALLOC, and GSTACK_FREE to avoid using 
 * malloc(), realloc(), and free().
 */

#ifndef GSTACK_DEF
    #ifdef GSTACK_STATIC
        #define GSTACK_DEF   static
    #else
        #define GSTACK_DEF   extern
    #endif                          /* GSTACK_STATIC */
#endif                              /* GSTACK_DEF */

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)
    #define ATTRIB_NONNULL(...)             __attribute__((nonnull(__VA_ARGS__)))
    #define ATTRIB_WARN_UNUSED_RESULT       __attribute__((warn_unused_result))
    #define ATTRIB_MALLOC                   __attribute__((malloc))
#else
    #define ATTRIB_NONNULL(...)             /* If only. */
    #define ATTRIB_WARN_UNUSED_RESULT       /* If only. */
    #define ATTRIB_MALLOC(...)              /* If only. */
#endif                          /* defined(__GNUC__) || defined(__clang__) defined(__INTEL_LLVM_COMPILER) */

#include <stddef.h>
#include <stdbool.h>

typedef struct gstack gstack;

/*
 * Creates a stack with `cap` elements of size `memb_size`. 
 *
 * The stack can only store one type of elements. It does not support
 * heterogeneuous types. 
 *
 * Returns a pointer to the stack on success, or NULL on failure to allocate 
 * memory.
 */
GSTACK_DEF gstack *gstack_create(size_t cap, size_t memb_size) 
    ATTRIB_WARN_UNUSED_RESULT ATTRIB_MALLOC;

/* 
 * Pushes an element to the top of the stack referenced by `s`. It automatically
 * resizes the stack if it is full.
 *
 * Whilst pushing an element, there's no need of a cast, as there is an implicit
 * conversion to and from a void *.
 *
 * On a memory allocation failure, it returns false. Else it returns true.
 */
GSTACK_DEF bool gstack_push(gstack *s, const void *data)
    ATTRIB_NONNULL(1, 2) ATTRIB_WARN_UNUSED_RESULT;

/*
 * Removes the topmost element of the stack referenced by `s` and returns it. 
 * If the stack is empty, it returns NULL.
 *
 * The returned element should be casted to a pointer of the type that was 
 * pushed on the stack, and then dereferenced.
 * 
 * Note that casting to a pointer of the wrong type is Undefined Behavior, and
 * so is dereferencing to performing arithmetic on a void *.
 */
GSTACK_DEF void *gstack_pop(gstack *s) ATTRIB_NONNULL(1);

/* 
 * Returns a pointer to the topmost element of the stack referenced by `s`
 * without removing it.  If the stack is empty, it returns NULL.
 */
GSTACK_DEF const void *gstack_peek(const gstack *s) ATTRIB_NONNULL(1);

/*
 * Returns true if the capacity of the stack referenced by `s` is full, or false
 * elsewise.
 */
GSTACK_DEF bool gstack_is_full(const gstack *s) ATTRIB_NONNULL(1);

/*
 * Returns true if the count of elements in the stack referenced by `s` is zero,
 * or false elsewise.
 */
GSTACK_DEF bool gstack_is_empty(const gstack *s) ATTRIB_NONNULL(1);

/* 
 * Returns the count of elements in the stack referenced by `s`.
 */
GSTACK_DEF size_t gstack_size(const gstack *s) ATTRIB_NONNULL(1);

/*
 * Destroys and frees all memory associated with the stack referenced by `s`.
 */
GSTACK_DEF void gstack_destroy(gstack *s) ATTRIB_NONNULL(1);

#endif                          /* GSTACK_H */

#ifdef GSTACK_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(GSTACK_MALLOC) != defined(GSTACK_REALLOC) || defined(GSTACK_REALLOC) != defined(GSTACK_FREE)
    #error  "Must define all or none of GSTACK_MALLOC, GSTACK_REALLOC, and GSTACK_FREE."
#endif

#ifndef GSTACK_MALLOC
    #define GSTACK_MALLOC(sz)       malloc(sz)
    #define GSTACK_REALLOC(p, sz)   realloc(p, sz)
    #define GSTACK_FREE(p)          free(p)
#endif

struct gstack {
    void *data;
    size_t size;
    size_t cap;
    size_t memb_size;
};

GSTACK_DEF bool gstack_is_full(const gstack *s)
{
    return s->size == s->cap;
}

GSTACK_DEF bool gstack_is_empty(const gstack *s)
{
    return s->size == 0;
}

GSTACK_DEF const void *gstack_peek(const gstack *s)
{
    if (gstack_is_empty(s)) {
        return NULL;
    }

    return (char *) s->data + (s->size - 1) * s->memb_size;
}

GSTACK_DEF gstack *gstack_create(size_t cap, size_t memb_size)
{
    if (cap == 0 || memb_size == 0 || cap > SIZE_MAX / memb_size) {
        return NULL;
    }

    gstack *const s = GSTACK_MALLOC(sizeof *s);

    if (s) {
        s->data = GSTACK_MALLOC(memb_size * cap);

        if (s->data) {
            s->size = 0;
            s->cap = cap;
            s->memb_size = memb_size;
        } else {
            free(s);
            return NULL;
        }
    }

    return s;
}

GSTACK_DEF bool gstack_push(gstack *s, const void *data)
{
    if (s->size >= s->cap) {
        const bool cond = s->cap > SIZE_MAX / 2;

        if (cond) {
            return false;
        }
        
        s->cap *= 2;

        if (s->cap > SIZE_MAX / s->memb_size) {
            return false;
        }

        void *const tmp = GSTACK_REALLOC(s->data, s->cap * s->memb_size);

        if (!tmp) {
            return false;
        }

        s->data = tmp;
    } 

    char *const target = (char *) s->data + (s->size * s->memb_size);

    memcpy(target, data, s->memb_size);
    return ++s->size;
}

GSTACK_DEF void *gstack_pop(gstack *s)
{
    if (gstack_is_empty(s)) {
        return NULL;
    }

    --s->size;
        
    /* Half the array size if it is too large, or when it is less than one-fourth
     * the array size. This is the approach CLRS suggests. 
     */
    if (s->size && (s->size <= s->cap / 4)) {
        const size_t new_cap = s->cap / 2;

        if (s->cap % 2) {
            s->cap += 1;
        }
        void *const tmp = realloc(s->data, s->memb_size * new_cap); 

        if (tmp) {
            s->data = tmp;
            s->cap = new_cap;
        } 
        /* Else do nothing. The original memory is left intact. */
    }
    
    return (char *) s->data + (s->size * s->memb_size);
}

GSTACK_DEF void gstack_destroy(gstack *s)
{
    GSTACK_FREE(s->data);
    GSTACK_FREE(s);
}

GSTACK_DEF size_t gstack_size(const gstack *s)
{
    return s->size;
}

#undef ATTRIB_NONNULL
#undef ATTRIB_WARN_UNUSED_RESULT       
#undef ATTRIB_MALLOC                   

#endif                          /* GSTACK_IMPLEMENTATION */

#ifdef TEST_MAIN

#include <assert.h>
#include <stdint.h>

int main(void)
{
    gstack *stack = gstack_create(SIZE_MAX - 1000, sizeof (size_t));
    assert(!stack);
    
    stack = gstack_create(10000, sizeof (size_t));
    assert(stack);
    assert(!gstack_is_full(stack));

    for (size_t i = 0; i < 200000; ++i) {
        assert(gstack_push(stack, &i));
    }
    
    assert(!gstack_is_empty(stack));
    assert(gstack_size(stack) == 200000);
    assert(*(size_t *) gstack_peek(stack) == 199999);

    for (size_t i = 199999; i < SIZE_MAX; i--) {
        assert(*(size_t *) gstack_peek(stack) == i);
        assert(*(size_t *) gstack_pop(stack) == i);
    }

    assert(gstack_is_empty(stack));
    assert(gstack_size(stack) == 0);
    gstack_destroy(stack);
    return EXIT_SUCCESS;
}

#endif                          /* TEST_MAIN */
