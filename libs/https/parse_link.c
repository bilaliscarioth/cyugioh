#include <stdint.h>
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

char**
parse_link(char *restrict link, bool* is_https) {
	/*
	  tmp[0] = protocol/port
	  tmp[1] = hostname 
	  tmp[2] = entrypoint
	  tmp[3] = NULL Terminator.
	 */
	char** tmp = (char**) calloc(4, sizeof(char*));
	
	if (tmp == NULL)
		err(1, "calloc:");

	char* cursor = parse_protocol(link, is_https);
	if (cursor == NULL)
		goto error1;

	if (*cursor == '\0')
	{
		puts("link is incomplete,\nexample: http(s)://domain.name/entrypoint");
		goto error2;
	}

	tmp[0] = (char*) calloc(10, sizeof(char));
	
	if ( tmp[0] == NULL)
		goto error1;
	
	tmp[1] = (char*) calloc(8192, sizeof(char));
	if (tmp[1] == NULL)
		goto error2;
	

	char* port = NULL;
	char* end_hostname = parse_domain_name(cursor, &port);

	if (port != NULL)
	{
		if ( end_hostname-port < 9)
			memcpy(tmp[0], port, (size_t)(end_hostname-port));
		else
		{
			printf("out of bound for port, exceed 9 digits.\n");
			goto error3;
		}
	}
	else
	{
		if (is_https)
			strlcpy(tmp[0], "443", 10);
		else
			strlcpy(tmp[0], "80", 10);
	}
	
	memcpy(tmp[1], cursor, (size_t)(end_hostname-cursor));
	cursor = end_hostname;
	
	if (*cursor == '\0')
		return tmp;

	tmp[2] = (char*) calloc(8192, sizeof(char));
	if (tmp[2] == NULL)
		goto error3;

	
	memcpy(tmp[2], cursor, (size_t) strlen(cursor));
	return tmp;
	
error3:
	free(tmp[1]);
error2:
	free(tmp[0]);
error1:
	free(tmp);
	return NULL;
}
