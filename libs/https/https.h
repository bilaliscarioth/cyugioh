#include <sys/queue.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <tls.h>

#ifndef __HTTPS_
#define __HTTPS_

struct http_body;

struct http_kv* create_pair(char* restrict, char* restrict);

char* kv_into_str(struct http_kv*);

char** parse_link(char* restrict, bool*);

struct http_kv* parse_header(char* buf);

int http_request(const char* restrict, const char* restrict, struct http_body*);

struct http_kv
{
    LIST_ENTRY(http_kv) link;
    char* key;
    char* value;
};

struct http_response
{
    uint8_t status;
    char    status_text[64];
    LIST_HEAD(,http_kv) *headers;
    char*   content;
    ssize_t len_content;
};

struct http_body
{
    void*  payload;
    size_t len_payload;

    enum
    {
        HTTP1,
        HTTP1_1,
        HTTP2,
        HTTP3
    } http_version;

    struct http_response* response;
    //
    struct tls*        tls;
    struct tls_config* config;

    LIST_HEAD(,http_kv) *header;
    bool debug;
};

#endif
