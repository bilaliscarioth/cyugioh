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
    {
        goto https_close_connection;
    }


    body->response = calloc(1, sizeof(struct http_response));
    if (body->response == NULL)
        goto https_close_connection;

    ssize_t d         = 0;
    char    tmp[1369] = { 0 };


    while (0 < (d = tls_read(body->tls, tmp, sizeof(tmp))))
    {
        if (tmp[0] == '0')
            break;
        body->response->len_content += d;
        char* tmp_buf = realloc(body->response->content,
                                body->response->len_content * sizeof(char));
        if (tmp_buf == NULL)
            goto https_free_content_response;

        body->response->content = tmp_buf;

        memcpy(body->response->content + body->response->len_content - d,
               tmp,
               (size_t) d);
		printf("%lu \n", d);
				
        memset(tmp, 0, sizeof(tmp) - 1);
		printf("%lu \n", d);
    }

	printf("%s \n", body->response->content);
    tls_close(body->tls);
    return 0;

https_free_content_response:
    free(body->response->content);
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
    bool   is_https = true;
    char** v        = parse_link((char*) link, &is_https);
    if (v == NULL)
        return -1;

	LIST_HEAD(,http_kv) *headers = calloc(1, sizeof(LIST_HEAD(, http_kv)));
	if (headers == NULL)
		goto free_v;

	
	LIST_INSERT_HEAD(headers, create_pair("Host", v[1]), link);

    if (body->header != NULL)
		LIST_INSERT_BEFORE(headers->lh_first, body->header->lh_first, link);
	
	
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
    char   req[8192];
    len = (size_t) snprintf(req, 8192, "%s %s %s\r\n", method, v[2], http_version);

	char* head_buffer =  kv_into_str(headers->lh_first);
	if (head_buffer == NULL)
		goto free_head;
	
	len += (size_t)snprintf(req+len, 8192-len, "\r\n\r\n");
    if (is_https && https(v, body, req, len) != 0)
		goto free_head_buffer;
    else if (!is_https && http(v, body, req, len) != 0)
		goto free_head_buffer;

    return 0;
free_head_buffer:
	free(head_buffer);
free_head:
	for (struct http_kv *kv = LIST_FIRST(headers); kv != NULL;)
	{
		struct http_kv* tmp = kv;
		kv = LIST_NEXT(kv, link);
		free(tmp);
	}
	free(headers);
free_v:
    for (uint8_t i = 0; i < 4; i++)
        free(v[i]);
    free(v);
    return 1;
}
