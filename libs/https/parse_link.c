#include "https.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

static char *
parse_protocol(char* restrict link, bool* https)
{
	char* cursor = link;
	
	if ( (cursor = strstr(link, "https://")) != NULL )
	{
		cursor += 8;
		*https = true;
	}
	
	else if ( (cursor = strstr(link, "http://")) != NULL)
	{
		cursor += 7;
		*https = false;
	}
	else
		return NULL;

	return cursor;
}

static char *
parse_domain_name(char* cursor, char** port)
{
	char* end_hostname = strstr(cursor, "/");
	char* beg_port = strstr(cursor, ":");

	// if the link is only https://domain.name 
	if (end_hostname == NULL)
	{
		const size_t len_link = strlen(cursor);
		end_hostname = cursor + len_link -1;
	}

	// Optional: taking port
	if (beg_port != NULL && beg_port < end_hostname)
		*port = beg_port;

	return end_hostname;
}

bool
parse_link(char* restrict link, struct http_link* format)
{
    if (format == NULL || link == NULL)
		return false;
	
    memset(format, 0, sizeof(struct http_link));
    char* cursor = parse_protocol(link, &(format->is_https));
	if (cursor == NULL)
		return false;

	if (*cursor == '\0')
	{
		puts("link is incomplete,\nexample: http(s)://domain.name/entrypoint");
		return false;
	}

	char* port = NULL;
    char* end_hostname = parse_domain_name(cursor, &port);
	
	if (port != NULL)
    {
		memcpy(format->host, cursor, (size_t)(port-cursor));
		if ( end_hostname-port < 7)
			memcpy(format->port, port, (size_t)(end_hostname-port));
		else
		{
			printf("out of bound for port, exceed 9 digits.\n");
			return false;
		}
	}
	else
    {
		memcpy(format->host, cursor, (size_t)(end_hostname-cursor));
		if (format->is_https)
			strlcpy(format->port, "443", 10);
		else
			strlcpy(format->port, "80", 10);
	}

	

	cursor = end_hostname;

    if (*cursor == '\0')
    {
		strcpy(format->entrypoint, "/");
        return true;
	}

	memcpy(format->entrypoint, cursor, (size_t) strlen(cursor));
	return true;
error:
	memset(format, 0, sizeof(struct http_link));
	return false;
}
