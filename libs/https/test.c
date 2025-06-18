#include "https.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tls.h>
#include <err.h>

int
main()
{
    bool is_https;
	struct http_link link;
    if (! parse_link(
			"https://db.ygoprodeck.com/api/v7/cardinfo.php?name=Slifer%20The%20Sky%20Dragon",
			&link))
        err(1, "parse_link:");

    assert(strncmp(link.port, "443", 3) == 0);
    assert(strncmp(link.host, "db.ygoprodeck.com", 17) == 0);
    assert(strncmp(link.entrypoint,
                   "/api/v7/cardinfo.php?name=Slifer%20The%20Sky%20Dragon",
                   53)
           == 0);
	
    if (tls_init() != 0)
        err(1, "tls_init:");

    struct tls* tls;

    if ((tls = tls_client()) == NULL)
        err(1, "tls_client:");

    struct http_body t = {
        .tls          = tls,
        .http_version = HTTP1_1,
        .response     = NULL,
        .config       = NULL,
        .payload      = NULL,
    };

    if (http_request(
            "GET",
            "https://db.ygoprodeck.com/api/v7/cardinfo.php?name=Slifer%20The%20Sky%20Dragon",
            &t)
        != 0)
	{
		tls_free(tls);
        return 1;
	}

    tls_free(tls);
	t.tls = NULL;

	if ( t.response != NULL)
	{
		printf("%s \n", t.response->content);
        free(t.response->content);
        FREE_HTTP_HEADERS(t.response->headers);
        free(t.response->headers);
	}

	free(t.response);
    return 0;
}
