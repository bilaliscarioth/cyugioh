#include <sys/queue.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <tls.h>

#ifndef __HTTPS_
#define __HTTPS_


struct http_body;
struct http_response;

struct http_kv* create_pair(char* restrict, char* restrict);

char* kv_into_str(struct http_kv*);

char** parse_link(char* restrict, bool*);

int parse_response(char*, struct http_response*);

int http_request(const char* restrict, const char* restrict, struct http_body*);

struct http_kv
{
    LIST_ENTRY(http_kv) link;
    char* key;
    char* value;
};

enum http_version
{
	HTTP1,
	HTTP1_1,
	HTTP2,
	HTTP3
};


struct http_response
{
	enum http_version http_version;
    uint8_t status;
    char    status_text[64];
    LIST_HEAD(,http_kv) *headers;
    char*   content;
    ssize_t len_content;
};

struct http_body
{
	
	enum http_version http_version;
    void*  payload;
    size_t len_payload;


    struct http_response* response;
    //
    struct tls*        tls;
    struct tls_config* config;

    LIST_HEAD(,http_kv) *header;
    bool debug;
};

#endif
