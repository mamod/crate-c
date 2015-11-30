#ifndef _COMO_CORE_H
#define _COMO_CORE_H

#include <errno.h>

#ifdef _WIN32
    #ifdef __TINYC__
        #include "tinycc/assert.h"
        #include "tinycc/winsock2.h"
        #include "tinycc/mswsock.h"
        #include "tinycc/ws2tcpip.h"
    #else
        #include <winsock2.h>
        #include <mswsock.h>
        #include <ws2tcpip.h>
        #include <assert.h>
    #endif
    
    #include <windows.h>

    #define COMO_GET_LAST_ERROR GetLastError()

    static inline int _WSLASTERROR (){
        int er = WSAGetLastError();
        if (er == WSAEINVAL){
            return EINVAL;
        }
        return er;
    }
    
    #define COMO_GET_LAST_WSA_ERROR _WSLASTERROR()
#else
    #include <assert.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <dlfcn.h>
    #define COMO_GET_LAST_ERROR errno

    /* win32 socket compatible constants */
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define COMO_GET_LAST_WSA_ERROR errno
#endif

#include "duktape/duktape.h"

void dump_stack(duk_context *ctx, const char *name) {
    duk_idx_t i, n;
    n = duk_get_top(ctx);
    printf("%s (top=%ld):", name, (long) n);
    for (i = 0; i < n; i++) {
        printf(" ");
        duk_dup(ctx, i);
        printf("%s", duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
    }
    printf("\n");
    fflush(stdout);
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define COMO_METHOD(name) static int (name)(duk_context *ctx)


#define COMO_SET_ERRNO(ctx, err) do \
{\
    errno = err; \
    duk_push_global_object(ctx); \
    duk_get_prop_string(ctx, -1, "process"); \
    duk_push_string(ctx, "errno"); \
    duk_push_int(ctx, err); \
    duk_put_prop(ctx, -3); \
} while(0)

#define COMO_SET_ERRNO_AND_RETURN(ctx, err) do \
{\
    COMO_SET_ERRNO(ctx,err);\
    duk_push_null(ctx);\
    return 1;\
} while(0)

#define COMO_FATAL_ERROR(err) do \
{                              \
    fprintf(stderr, "\n\n--> ");   \
    fprintf(stderr, err);      \
    fprintf(stderr, "\n\n");     \
    assert(0); \
} while(0)


void como_run (duk_context *ctx);
duk_context *como_create_new_heap (int argc, char *argv[], void *error_handle);

void como_sleep (int timeout){
    #ifdef _WIN32
        Sleep(timeout);
    #else
        usleep(1000 * timeout);
    #endif
}

#endif /*_COMO_CORE_H*/
