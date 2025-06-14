#include "https.h"
#include <arpa/inet.h>

#include <stdio.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tls.h>


static int
http(char** format, struct http_body* body, char* req, size_t len)
{
    (void) format;
    (void) body;
    (void) req;
    (void) len;
    return 0;
}

static int
https(char** format, struct http_body* body, char* req, size_t len)
{
    if (tls_connect(body->tls, format[1], format[0]) != 0)
        return 1;

    if ((size_t) tls_write(body->tls, req, len) != len)
        goto https_close_connection;

    ssize_t d         = 0;
    char    tmp[1369] = { 0 };
    char* rep_buffer = NULL;
	size_t rep_buffer_len = 0;
	
	body->response = calloc(1, sizeof(struct http_response));
    if (body->response == NULL)
        goto https_close_connection;

    while (0 < (d = tls_read(body->tls, tmp, sizeof(tmp))))
    {
        if (tmp[0] == '0')
            break;
		
        rep_buffer_len += d;
        char* tmp_buf = realloc(rep_buffer, rep_buffer_len * sizeof(char));
		
        if (tmp_buf == NULL)
            goto https_free_content_response;

        rep_buffer = tmp_buf;

        memcpy(rep_buffer + rep_buffer_len - d,
               tmp,
               (size_t) d);
        memset(tmp, 0, sizeof(tmp) - 1);
    }

	if ( parse_response(rep_buffer, body->response) != 0)
        goto https_free_content_response;

    free(rep_buffer);

    tls_close(body->tls);
    return 0;
	// Unreachable
	//Error handling;
https_free_content_response:
    free(rep_buffer);
	free(body->response);
https_close_connection:
    tls_close(body->tls);
    return 1;
}

int
http_request(const char* restrict method,
             const char* restrict link,
             struct http_body* body)
{
	char*  head_buffer = NULL;
    bool   is_https = true;
    char** v           = parse_link((char*) link, &is_https);
    LIST_HEAD(, http_kv) headers;
    LIST_INIT(&headers);
	
	// If we can't parse links.
    if (v == NULL)
        return 1;
	
    LIST_INSERT_HEAD(&headers, create_pair("Host", v[1]), link);
	
    if (LIST_EMPTY(&headers))
		goto free_head;

	// Add others headers.
    if (body->header != NULL)
		LIST_INSERT_AFTER(LIST_FIRST(&headers), body->header->lh_first, link);
	
	
    char http_version[16] = { 0 };
	// No need of strlcpy nor strncpy because
	// 16 chars is sufficient to contain each cases.
    switch (body->http_version)
    {
        case HTTP1:
            strcpy(http_version, "HTTP/1");
            break;
        case HTTP1_1:
            strcpy(http_version, "HTTP/1.1");
            break;
        case HTTP2:
            strcpy(http_version, "HTTP/2");
            break;
        case HTTP3:
            strcpy(http_version, "HTTP/3");
            break;
    }

    size_t len = 8192;
    char   req[8192] = {0};
    len = (size_t) snprintf(req, 8192, "%s %s %s\r\n", method, v[2], http_version);

	head_buffer =  kv_into_str(LIST_FIRST(&headers));
	if (head_buffer == NULL)
		goto free_head;

	// free list.
	while ( !LIST_EMPTY(&headers))
    {
        struct http_kv* tmp = LIST_FIRST(&headers);
		if (LIST_NEXT(tmp, link) != NULL)
            LIST_REMOVE(tmp, link);
		else
			headers.lh_first = NULL;
        free(tmp->key);
		free(tmp->value);
		free(tmp);
	}
	
	len += (size_t)snprintf(req+len, 8192-len, "%s", head_buffer);
		
    if (is_https && https(v, body, req, len) != 0)
		goto free_head_buffer;
    else if (!is_https && http(v, body, req, len) != 0)
		goto free_head_buffer;

	//  free what we used
    free(head_buffer);
	head_buffer = NULL;
    for (uint8_t i = 0; i < 4; i++)
        free(v[i]);
    free(v);
	v = NULL;
    return 0;
	// Unreachable
	//Error handling;
free_head:
	
	// free list.
	while ( !LIST_EMPTY(&headers))
    {
        struct http_kv* tmp = LIST_FIRST(&headers);
		if (LIST_NEXT(tmp, link) != NULL)
			LIST_REMOVE(tmp, link);
        free(tmp->key);
		free(tmp->value);
		free(tmp);
	}
free_head_buffer:
	free(head_buffer);
free_v:
    for (uint8_t i = 0; i < 4; i++)
        free(v[i]);
    free(v);
    return 1;
}
