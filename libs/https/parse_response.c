#include "https.h"

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/queue.h>

static char*
get_http_version(char* buf, struct http_response* response)
{
    char* cur;
    char* end = strstr(buf, "\r\n");
	
    if (end == NULL)
		return NULL;
	
    if ((cur = strstr(buf, "HTTP/1 ")) != NULL)
    {
		response->http_version = HTTP1;
		cur += 7;
    }
    else if ((cur = strstr(buf, "HTTP/1.1 ")) != NULL)
    {
		response->http_version = HTTP1_1;
		cur +=  9;
    }
    else if ((cur = strstr(buf, "HTTP/2 ")) != NULL)
    {
		response->http_version = HTTP2;
		cur += 7;
    }
    else if ((cur = strstr(buf, "HTTP/3 ")) != NULL)
    {
		response->http_version = HTTP3;
		cur += 7;
    }
    else
		return NULL;
	return cur;
}

int
parse_response(char* buf, struct http_response* response)
{
	if (buf == NULL)
		return 1;

	response->headers = calloc(1, sizeof(LIST_HEAD(, http_kv)));
	if (response->headers == NULL)
        return 1;

    char* line = get_http_version(buf, response);
    if (line == NULL)
		goto free_elements;

	// GET HTTP first line
    char* http_status_index = strstr(line, " ");
    char  status[8]         = { 0 };

    memcpy(status, line, 3);

    char* end = strstr(http_status_index, "\r\n");
    if (end - http_status_index > 62)
        goto free_elements;
	
    memcpy(response->status_text, http_status_index + 1, end-http_status_index-1);
	
    const char* strtonum_err = NULL;
    response->status         = strtonum(status, 100, 599, &strtonum_err);
	
    if (strtonum_err != NULL)
        goto free_elements;

    // Parsing headers
    // Key: Val


    end = strstr(line, "\r\n");
    line      = end + 2;
	
    struct http_kv* current_node = NULL;
 	while ( (end = strstr(line, "\r\n")) != NULL)
    {
        char* sep = strstr(line, ":");
        if (sep == NULL || sep > end)
			break;

		char key[1024] = {0};
        char val[1024] = {0};

        if (sep - line > 1023 || (end-sep-2) > 1023)
            goto free_elements;
		
        memcpy(key, line, (size_t) (sep - line));
        memcpy(val, sep + 2, (size_t) (end - sep) - 2);

        if (strcmp(key, "Connection") == 0 && strcmp(val, "close") == 0)
			goto free_elements;

        struct http_kv* tmp = create_pair(key, val);
		if (tmp == NULL)
			goto free_elements;

		// Add each HTTP Header values;
		if (LIST_EMPTY(response->headers))
			LIST_INSERT_HEAD(response->headers, tmp, link);
		else
			LIST_INSERT_AFTER(current_node, tmp, link);

        current_node = tmp;
		line = end + 2; // nextline
    }
	
    struct http_kv* find;
    LIST_FOREACH(find, response->headers, link)
    {
        if (strcmp(find->key, "Transfer-Encoding") == 0)
            break;
    }

	if (strcmp(find->value, "chunked") == 0)
    {
		//skip blank field;
		line += 2;
        while (*line != 0)
        {
            // Read size
            end = strstr(line, "\r\n");
            char size_str[16] = { 0 };
            if (end - line > 15)
                goto free_elements;

            memcpy(size_str, line, end - line);
			const char* strtonum_err = NULL;

			char *ep = NULL;
            const long d  = strtol(size_str, &ep, 16);
			
			if (size_str[0] == '\0' || *ep != '\0')
				goto free_elements;
			if (errno == ERANGE && (d == LONG_MAX || d == LONG_MIN))
				goto free_elements;

			// skip <size>\r\n;
            line = end + 2;
			response->len_content += d;
            // Read content;
			char* tmp  = realloc(response->content, response->len_content);
			if (tmp == NULL)
            	goto free_elements;

            response->content = tmp;
            int readed        = 0;

			memcpy(response->content, line, d);
			line += d;
			line += 2;
        }
    }
	
	return 0;
free_elements:
	FREE_HTTP_HEADERS(response->headers);
    free(response->headers);
    response->headers = NULL;

    free(response->content);
	response->content = NULL;
	response->len_content = 0;

	return 1;
}
