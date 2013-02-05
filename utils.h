#ifndef __UTILS_H_
#define __UTILS_H_

#ifndef container_of
/* TODO: Add a type-equals-compile-time-assertion on memberptr? */
#define container_of(memberptr, containername, membername)  \
    ((containername *)((char*)(memberptr) - offsetof(containername, membername)))
#endif

#ifndef ARRAY_LEN
#define ARRAY_LEN(arr)    (sizeof (arr) / sizeof *(arr))
#endif

#define min(a, b)   (((a) <= (b)) ? (a) : (b))
#define max(a, b)   (((a) >= (b)) ? (a) : (b))

#endif
