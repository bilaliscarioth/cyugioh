#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include "https.h"

char*
kv_into_str(struct http_kv* headers)
{
	if (headers == NULL)
		return NULL;
	
	size_t len = 0;
	char* buffer = NULL;
	
	for(struct http_kv* kv = headers; kv != NULL; kv = LIST_NEXT(kv, link))
	{
		char b[1024] = {0};
		int d= snprintf(b, 1024, "%s: %s\r\n", kv->key, kv->value);
		if ( d == -1)
			goto error;
			
		len += (size_t)d;
		char* tmp = realloc(buffer, len * sizeof(char) + 1);
		if (tmp == NULL)
			goto error;

		buffer = tmp;
		memcpy(buffer + len -d, tmp, (size_t)d);
	}
	buffer[len] = '\0';

    return buffer;
error:
	free(buffer);
	return NULL;
}
