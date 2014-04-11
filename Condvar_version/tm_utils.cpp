// [branch 012] Include support for registering onCommit handlers:
#include "/home/chao/gcc/src/gcc_trunk/libitm/libitm.h"
#include "tm_utils.h"
#include "memcached.h"

// [branch 008] This is our 'safe' assert function...
#if !defined(NDEBUG)
void tm_assert_internal(const char *filename, int linenum, const char *funcname, const char *sourceline)
{
    // [mfs] This isn't entirely correct... the formatting is wrong for
    // Solaris, and on Linux we are supposed to also print the name of the
    // executable... we can live without such touch-ups for now...
    fprintf(stderr, "%s:%d: %s: Assertion '%s' failed.\n", filename, linenum, funcname, sourceline);
    abort();
}
#endif

// [branch 008] This is our 'safe' printf-and-abort function
void tm_msg_and_die(const char* msg)
{
    fprintf(stderr, msg);
    abort();
}

// [branch 009] Provide an implementation of memcmp that is transaction_safe
//
// NB: this code was taken from
// http://doxygen.postgresql.org/memcmp_8c_source.html, and is covered by a
// BSD license
int tm_memcmp(const void *s1, const void *s2, size_t n)
{
    if (n != 0) {
        const unsigned char *p1 = s1, *p2 = s2;
        do
        {
            if (*p1++ != *p2++)
                return (*--p1 - *--p2);
        } while (--n != 0);
    }
    return 0;
}

// [branch 009] Provide an implementation of memcpy that is transaction_safe
//
// NB: this code was taken from
// https://www.student.cs.uwaterloo.ca/~cs350/common/os161-src-html/memcpy_8c-source.html
void * tm_memcpy(void *dst, const void *src, size_t len)
{
    size_t i;
    /*
     * memcpy does not support overlapping buffers, so always do it
     * forwards. (Don't change this without adjusting memmove.)
     *
     * For speedy copying, optimize the common case where both pointers
     * and the length are word-aligned, and copy word-at-a-time instead
     * of byte-at-a-time. Otherwise, copy by bytes.
     *
     * The alignment logic below should be portable. We rely on
     * the compiler to be reasonably intelligent about optimizing
     * the divides and modulos out. Fortunately, it is.
     */
    if ((uintptr_t)dst % sizeof(long) == 0 &&
        (uintptr_t)src % sizeof(long) == 0 &&
        len % sizeof(long) == 0)
    {
        long *d = dst;
        const long *s = src;
        for (i=0; i<len/sizeof(long); i++) {
            d[i] = s[i];
        }
    }
    else {
        char *d = dst;
        const char *s = src;
        for (i=0; i<len; i++) {
            d[i] = s[i];
        }
    }
    return dst;
}

// [branch 009] provide a safe implementation of strlen
//
// NB: this code is taken from
// https://www.student.cs.uwaterloo.ca/~cs350/common/os161-src-html/strlen_8c-source.html
size_t tm_strlen(const char *s)
{
    size_t ret = 0;
    while (s[ret]) {
        ret++;
    }
    return ret;
}

// [branch 009] provide a safe implementation of strncmp
//
// NB: this code is taken from
// http://www.ethernut.de/api/strncmp_8c_source.html
int tm_strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++)
            return (*(unsigned char *) s1 - *(unsigned char *) --s2);
        if (*s1++ == 0)
            break;
    } while (--n != 0);
    return (0);
}

// [branch 009] Provide a safe strncpy function
//
// NB: this code is taken from
// http://www.ethernut.de/api/strncpy_8c_source.html
char *tm_strncpy(char *dst, const char *src, size_t n)
{
    if (n != 0) {
        char *d = dst;
        const char *s = src;
        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n) {
                    *d++ = 0;
                }
                break;
            }
        } while (--n);
    }
    return dst;
}

// [branch 009] Provide a transaction-safe version of realloc
void *tm_realloc(void *ptr, size_t size, size_t old_size)
{
    void *oldp = ptr;
    void* newp = malloc(size);
    if (newp == NULL)
        return NULL;
    size_t copySize = *(size_t *)((char *)oldp - sizeof(size_t));
    if (size < copySize)
        copySize = size;
    if (old_size > 0) {
        tm_memcpy(newp, oldp, old_size);
    }
    free(oldp);
    return newp;
}

// [branch 009b] Provide a safe version of strchr.
// This code taken from http://clc-wiki.net/wiki/strchr
char *tm_strchr(const char *s, int c)
{
    while (*s != (char)c)
        if (!*s++)
            return 0;
    return (char *)s;
}

// [branch 009b] Safe isspace
int tm_isspace(int c)
{
    return isspace(c);
}

// [branch 009b] Provide a pure memcpy, so that we can do calle stack -to-
//               caller stack transfers without having the writes getting
//               buffered by the TM
__attribute__((transaction_pure))
static void *pure_memcpy(void *dest, const void *src, size_t n)
{
    return memcpy(dest, src, n);
}

// [branch 009b] Provide a safe strncpy function that copies its argument to
//               a local array instead of to a possibly-shared array
//
// NB: this code is taken from
// http://www.ethernut.de/api/strncpy_8c_source.html
#define MAX(x, y) ((x)>(y)) ? (x) : (y)
char *tm_strncpy_to_local(char *local_dst, const char *src, size_t n)
{
    // keep track of the size, and have a local destination so that gcc
    // doesn't do transactional writes, only transactional reads
    int size = n;
    char dst[MAX(KEY_MAX_LENGTH + 1, STAT_VAL_LEN) + 1];

    if (n != 0) {
        char *d = dst;
        const char *s = src;
        do {
            if ((*d++ = *s++) == 0) {
                /* NUL pad the remaining n-1 bytes */
                while (--n) {
                    *d++ = 0;
                }
                break;
            }
        } while (--n);
    }

    // now we need to copy from dst (which is local to this function) to
    // local_dst (which is local to the caller)
    pure_memcpy(local_dst, dst, size);

    return local_dst;
}

// [branch 009b] Safe strtol via marshalling
__attribute__((transaction_pure))
static long int pure_strtol(const char *nptr, char **endptr, int base)
{
    return strtol(nptr, endptr, base);
}
// NB: memcached always sends NULL for endptr, so we don't have to do
// anything fancy here
long int tm_strtol(const char *nptr, char **endptr, int base)
{
    // marshall string into buffer... the biggest 64-bit int is
    // 9,223,372,036,854,775,807, so 19 characters.  Note that the string can
    // have a ton of leading whitespace, so we'll go with 4096 to play it
    // safe
    char buf[4096];
    tm_strncpy_to_local(buf, nptr, 4096);
    // now go to a pure function call
    return pure_strtol(buf, endptr, base);
}

// [branch 009b] Since uses of strtoull need endptr, we re-produce the code
// instead of marshalling.  Code is taken from
// http://ftp.cc.uoc.gr/mirrors/OpenBSD/src/lib/libc/stdlib/strtoull.c
unsigned long long int tm_strtoull(const char *nptr, char **endptr, int base)
{
    const char *s;
    unsigned long long acc, cutoff;
    int c;
    int neg, any, cutlim;

    /*
     * See strtoq for comments as to the logic used.
     */
    s = nptr;
    do {
        c = (unsigned char) *s++;
    } while (isspace(c));
    if (c == '-') {
        neg = 1;
        c = *s++;
    } else {
        neg = 0;
        if (c == '+')
            c = *s++;
    }
    if ((base == 0 || base == 16) &&
        c == '0' && (*s == 'x' || *s == 'X')) {
        c = s[1];
        s += 2;
        base = 16;
    }
    if (base == 0)
        base = c == '0' ? 8 : 10;

    cutoff = ULLONG_MAX / (unsigned long long)base;
    cutlim = ULLONG_MAX % (unsigned long long)base;
    for (acc = 0, any = 0;; c = (unsigned char) *s++) {
        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= isupper(c) ? 'A' - 10 : 'a' - 10;
        else
            break;
        if (c >= base)
            break;
        if (any < 0)
            continue;
        if (acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
            acc = ULLONG_MAX;
            errno = ERANGE;
        } else {
            any = 1;
            acc *= (unsigned long long)base;
            acc += c;
        }
    }
    if (neg && any > 0)
        acc = -acc;
    if (endptr != 0)
        *endptr = (char *) (any ? s - 1 : nptr);
    return (acc);
}

// [branch 009b] This is how BSD does atoi...
int tm_atoi(const char *nptr)
{
    return((int)tm_strtol(nptr, (char **)NULL, 10));
}

// [branch 012] Provide support for oncommit handlers
void registerOnCommitHandler(void (*func)(void*), void *param)
{
    if (_ITM_inTransaction() == outsideTransaction) {
        func(param);
    }
    else {
        _ITM_addUserCommitAction((_ITM_userCommitFunction)func,
                                 //_ITM_getTransactionId(),
                                 1,
                                 param);
    }
}
