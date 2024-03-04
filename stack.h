#ifndef STACK_H
#define STACK_H

/* To use, do this:
 *  #define STACK_IMPLEMENTATION
 * before you include this file in *one* C file to create the implementation.
 *
 * i.e. it should look like:
 * #include ...
 * #include ...
 *
 * #define STACK_IMPLEMENTATION
 * #include "stack.h"
 * ...
 *
 * To make all the functions have internal linkage, i.e. be private to the
 * source file, do this:
 * #define IO_STATIC
 * before including "stack.h"
 *
 * i.e. it should look like:
 * #define STACK_IMPLEMENTATION
 * #define STACK_STATIC
 * #include "stack.h"
 * ...
 *
 * You can define STACK_MALLOC, STACK_REALLOC, and STACK_FREE to avoid using 
 * malloc(), realloc(), and free().
 */

#ifndef STACK_DEF
    #ifdef STACK_STATIC
        #define STACK_DEF   static
    #else
        #define STACK_DEF   extern
    #endif                          /* STACK_STATIC */
#endif                              /* STACK_DEF */

#if defined(__GNUC__) || defined(__clang__)
    #define ATTRIB_NONNULL(...)             __attribute__((nonnull(__VA_ARGS__)))
    #define ATTRIB_WARN_UNUSED_RESULT       __attribute__((warn_unused_result))
    #define ATTRIB_MALLOC                   __attribute__((malloc))
#else
    #define ATTRIB_NONNULL(...)             /* If only. */
    #define ATTRIB_WARN_UNUSED_RESULT       /* If only. */
    #define ATTRIB_MALLOC(...)              /* If only. */
#endif                          /* defined(__GNUC__) || defined(__clang__) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct stack Stack;

/*
 * Creates a stack with `cap` elements of size `memb_size`. 
 *
 * The stack can only store one type of elements. It does not support
 * heterogeneuous types. 
 *
 * Returns a pointer to the stack on success, or NULL on failure to allocate 
 * memory.
 */
STACK_DEF Stack *stack_create(size_t cap, size_t memb_size) 
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
STACK_DEF bool stack_push(Stack *s, const void *data)
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
STACK_DEF void *stack_pop(Stack *s) ATTRIB_NONNULL(1);

/* 
 * Returns a pointer to the topmost element of the stack referenced by `s`
 * without removing it.  If the stack is empty, it returns NULL.
 */
STACK_DEF const void *stack_peek(const Stack *s) ATTRIB_NONNULL(1);

/*
 * Returns true if the capacity of the stack referenced by `s` is full, or false
 * elsewise.
 */
STACK_DEF bool stack_is_full(const Stack *s) ATTRIB_NONNULL(1);

/*
 * Returns true if the count of elements in the stack referenced by `s` is zero,
 * or false elsewise.
 */
STACK_DEF bool stack_is_empty(const Stack *s) ATTRIB_NONNULL(1);

/* 
 * Returns the count of elements in the stack referenced by `s`.
 */
STACK_DEF size_t stack_size(const Stack *s) ATTRIB_NONNULL(1);

/*
 * Destroys and frees all memory associated with the stack referenced by `s`.
 */
STACK_DEF void stack_destroy(Stack *s) ATTRIB_NONNULL(1);

#endif                          /* STACK_H */

#ifdef STACK_IMPLEMENTATION

#if defined(STACK_MALLOC) && defined(STACK_REALLOC) && defined(STACK_FREE)
// Ok.
#elif !defined(STACK_MALLOC) && !defined(STACK_REALLOC) && !defined(STACK_FREE)
// Ok.
#else
    #error  "Must define all or none of STACK_MALLOC, STACK_REALLOC, and STACK_FREE."
#endif

#ifndef STACK_MALLOC
    #define STACK_MALLOC(sz)       malloc(sz)
    #define STACK_REALLOC(p, sz)   realloc(p, sz)
    #define STACK_FREE(p)          free(p)
#endif

struct stack {
    void *data;
    size_t size;
    size_t cap;
    size_t memb_size;
};

STACK_DEF bool stack_is_full(const Stack *s)
{
    return s->size == s->cap;
}

STACK_DEF bool stack_is_empty(const Stack *s)
{
    return s->size == 0;
}

STACK_DEF const void *stack_peek(const Stack *s)
{
    if (stack_is_empty(s)) {
        return NULL;
    }

    return (char *) s->data + (s->size - 1) * s->memb_size;
}

STACK_DEF Stack *stack_create(size_t cap, size_t memb_size)
{
    if (cap == 0 || memb_size == 0 || cap > SIZE_MAX / memb_size) {
        return NULL;
    }

    Stack *const s = STACK_MALLOC(sizeof *s);

    if (s) {
        /* Would it be an improvement to round this up to the nearest
         * multiple/power of 2.
         */
        size_t total_size = memb_size * cap;
        s->data = STACK_MALLOC(total_size);

        if (s->data) {
            s->cap = cap;
            s->size = 0;
            s->memb_size = memb_size;
        } else {
            free(s);
            return NULL;
        }
    }

    return s;
}

STACK_DEF bool stack_push(Stack *s, const void *data)
{
    if (s->size >= s->cap) {
        /* If we cannot allocate geometrically, we shall allocate linearly. */ 
        if (s->cap > SIZE_MAX / 2) {
            if (s->cap + BUFSIZ < s->cap) {
                return false;
            }
            s->cap += BUFSIZ;
        } else {
            s->cap *= 2;
        }

        if (s->cap > SIZE_MAX / s->memb_size) {
            return false;
        }

        void *const tmp = STACK_REALLOC(s->data, s->cap * s->memb_size);

        if (!tmp) {
            return false;
        }

        s->data = tmp;
    } 

    char *const target = (char *) s->data + (s->size * s->memb_size);

    memcpy(target, data, s->memb_size);
    return !!++s->size;
}

STACK_DEF void *stack_pop(Stack *s)
{
    if (stack_is_empty(s)) {
        return NULL;
    }

    --s->size;
    void *const top = (char *) s->data + (s->size * s->memb_size);
        
    if (s->size && (s->size <= s->cap / 4)) {
        void *const tmp = realloc(s->data, s->cap / 2 * s->memb_size);
        
        if (tmp) {
            s->data = tmp;
            s->cap /= 2;
        } 
        /* Else do nothing. The original memory is left intact. */
    }
    
    return top;
}

STACK_DEF void stack_destroy(Stack *s)
{
    STACK_FREE(s->data);
    STACK_FREE(s);
}

STACK_DEF size_t stack_size(const Stack *s)
{
    return s->size;
}

#endif                          /* STACK_IMPLEMENTATION */

#ifdef TEST_MAIN

#include <assert.h>

int main(void)
{
    /* We could support heterogenuous objects by using void pointers. */
    Stack *stack = stack_create(SIZE_MAX - 1000, sizeof (size_t));
    assert(!stack);

    stack = stack_create(1000, sizeof (int));
    assert(stack);

    for (int i = 0; i < 150; ++i) {
        assert(stack_push(stack, &i));
    }
    
    assert(!stack_is_empty(stack));
    assert(stack_size(stack) == 150);
    assert(*(int *) stack_peek(stack) == 149);

    for (int i = 149; i >= 0; i--) {
        assert(*(int *) stack_peek(stack) == i);
        assert(*(int *) stack_pop(stack) == i);
    }
    stack_destroy(stack);
    return EXIT_SUCCESS;
}

#endif                          /* TEST_MAIN */
