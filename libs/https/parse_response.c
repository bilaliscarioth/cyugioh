#include "https.h"
#include <string.h>
#include <stdio.h>
#include <sys/queue.h>

int
parse_response(char* buf, struct http_response* response)
{
	if (buf == NULL)
		return 1;

	response->headers = calloc(1, sizeof(LIST_HEAD(, http_kv)));
	if (response->headers == NULL)
        return 1;

    char* line = buf;
    char* end  = strstr(line, "\r\n");
    if (end == NULL)
        goto free_elements;

    char* cur;
    if ((cur = strstr(line, "HTTP/1 ")) != NULL)
    {
		response->http_version = HTTP1;
		cur += 7;
    }
    else if ((cur = strstr(line, "HTTP/1.1 ")) != NULL)
    {
		response->http_version = HTTP1_1;
		cur +=  9;
    }
    else if ((cur = strstr(line, "HTTP/2 ")) != NULL)
    {
		response->http_version = HTTP2;
		cur += 7;
    }
    else if ((cur = strstr(line, "HTTP/3 ")) != NULL)
    {
		response->http_version = HTTP3;
		cur += 7;
    }
    else
		goto free_elements;

    char* http_status_index = strstr(cur, " ");
    char  status[8]         = { 0 };

    memcpy(status, cur, 3);
    memcpy(response->status_text, http_status_index + 1, sizeof(response->status_text));

    const char* strtonum_err = NULL;
    response->status         = strtonum(status, 100, 599, &strtonum_err);
	
    if (strtonum_err != NULL)
        goto free_elements;

    // Parsing headers
    // Key: Val
    
	
    line = end + 2; // skip '\r', \n'.
    struct http_kv* current_node = NULL;

 	while ( (end = strstr(line, "\r\n")) != NULL)
	{
        char* sep = strstr(line, ":");
        if (sep == NULL || sep > end)
			break;

		char key[1024] = {0};
		char val[1024] = {0};

		memcpy(key, line,(size_t)(sep-line));
		memcpy(val, sep+2, (size_t)(end-sep)-2);

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
	
	return 0;
free_elements:
	while(!LIST_EMPTY(response->headers))
	{
		struct http_kv* tmp= LIST_FIRST(response->headers);
        LIST_REMOVE(tmp, link);
		free(tmp);
	}
    free(response->headers);
	return 1;
}
