#include "https.h"
#include <stdlib.h>
#include <string.h>


struct http_kv*
create_pair(char* restrict key, char* restrict value)
{
	if (key == NULL || value == NULL)
		return NULL;

	// No need to set link field to NULL, calloc do it. 
	struct http_kv *pair= calloc(1, sizeof(struct http_kv));
	if (pair == NULL)
		return NULL;

	pair->key = strdup(key);
	pair->value = strdup(value);

	if (pair->key == NULL || pair->value == NULL)
	{
		free(pair->key);
		free(pair->value);
		free(pair);
		return NULL;
	}
	
	return pair;
}
