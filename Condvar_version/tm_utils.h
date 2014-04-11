#ifndef TM_UTILS_H
#define TM_UTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <event.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <ctype.h>
#include <stdarg.h>
#include <pwd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <sysexits.h>
#include <stddef.h>
// [branch 008] Support for safe assertions.  Note that the evaluation of the
// expression occurs within the context of the transaction, but we don't
// commit before we call the safe_assert_internal code.
#if defined(NDEBUG)
#define tm_assert(e)   ((void)0)
#else
#include <stdlib.h>
#define tm_assert(e) ((e) ? (void)0 : tm_assert_internal(__FILE__, __LINE__, __func__, #e))
__attribute__((transaction_pure))
void tm_assert_internal(const char *filename, int linenum, const char *funcname, const char *sourceline);
#endif /* NDEBUG */

// [branch 008] This is our 'safe' printf-and-abort function
__attribute__((transaction_pure))
void tm_msg_and_die(const char* msg);

// [branch 009] Provide safe versions of memcmp, memcpy, strlen, strncmp,
//              strncpy, and realloc
__attribute__((transaction_safe))
int tm_memcmp(const void *s1, const void *s2, size_t n);
__attribute__((transaction_safe))
void *tm_memcpy(void *dst, const void *src, size_t len);
__attribute__((transaction_safe))
size_t tm_strlen(const char *s);
__attribute__((transaction_safe))
int tm_strncmp(const char *s1, const char *s2, size_t n);
__attribute__((transaction_safe))
char *tm_strncpy(char *dst, const char *src, size_t n);
__attribute__((transaction_safe))
void *tm_realloc(void *ptr, size_t size, size_t old_size);

// [branch 009b] a custom strncpy that reads via TM, and writes directly to a
//               location presumed to be thread-private (e.g., on the stack)
__attribute__((transaction_safe))
char *tm_strncpy_to_local(char *local_dst, const char *src, size_t n);

// [branch 009b] Provide safe versions of strtol, atoi, strtol, strchr, and
//               isspace
__attribute__((transaction_safe))
long int tm_strtol(const char *nptr, char **endptr, int base);
__attribute__((transaction_safe))
int tm_atoi(const char *nptr);
__attribute__((transaction_safe))
unsigned long long int tm_strtoull(const char *nptr, char **endptr, int base);
__attribute__((transaction_safe))
char *tm_strchr(const char *s, int c);
// [branch 009b] This can just be marked pure
__attribute__((transaction_pure))
int tm_isspace(int c);

// [branch 012] Provide support for oncommit handlers
__attribute__((transaction_pure))
void registerOnCommitHandler(void (*func)(void*), void *param);

#endif
