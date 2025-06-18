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
http(const struct http_link *link, struct http_body* body, char* req, size_t len)
{
    (void) link;
    (void) body;
    (void) req;
    (void) len;
    return 0;
}

static int
https(const struct http_link *link, struct http_body* body, char* req, size_t len)
{
    if (tls_connect(body->tls, link->host, link->port) != 0)
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
    struct http_link parsed_link;
    if (!parse_link((char*) link, &parsed_link))
        return 1;
	
	char*  head_buffer = NULL;
	LIST_HEAD(, http_kv) headers;
    LIST_INIT(&headers);

    LIST_INSERT_HEAD(&headers, create_pair("Host", parsed_link.host), link);
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
    len = (size_t) snprintf(req, 8192, "%s %s %s\r\n", method, parsed_link.entrypoint, http_version);

	head_buffer =  kv_into_str(LIST_FIRST(&headers));
	if (head_buffer == NULL)
		goto free_head;

	// free list.
	FREE_HTTP_HEADERS((&headers));

	len += (size_t)snprintf(req+len, 8192-len, "%s", head_buffer);
		
    if (parsed_link.is_https && https(&parsed_link, body, req, len) != 0)
		goto free_head_buffer;
    else if (!parsed_link.is_https && http(&parsed_link, body, req, len) != 0)
		goto free_head_buffer;

	//  free what we used
    free(head_buffer);
	head_buffer = NULL;

    return 0;
	// Unreachable
	//Error handling;
free_head:
	// free list.
	FREE_HTTP_HEADERS((&headers));
free_head_buffer:
	free(head_buffer);
    return 1;
}
