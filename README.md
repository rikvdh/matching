# matching

[![Build Status](https://travis-ci.org/rikvdh/matching.svg?branch=master)](https://travis-ci.org/rikvdh/matching)

String-based parser based on matching strings for interpreting and handling string-based interfaces (like GSM-modem AT commands)

## Installation


With [clib](https://github.com/clibs/clib):

```sh
clib install rikvdh/matching
```

## Example

```c
#include "matching.h"
#include <assert.h>
#include <stdio.h>

static struct matching_ctx ctx;
static char ctx_linebuffer[256];

static void matching_cb_match_ko(struct matching_ctx *ctx)
{
	(void)ctx;
	printf("Matched on 'TEST'\n");
}

static const struct matching_item matching_items[] = {
	{ MATCHING_ITEM_STR("TEST"), 0, match_cb }
};

int main(int argc, char **argv)
{
	ctx.cfg.flags       = MATCHING_ITEM_FLAG_CB_ON_RESET;
	ctx.cfg.reset_chars = "\n";
	za_buffer_init(&ctx.linebuffer, ctx_linebuffer, sizeof(ctx_linebuffer));
	ctx.items.list      = matching_items;
	ctx.items.n         = sizeof(matching_items)/sizeof(matching_items[0]);
	ctx.items.state     = matching_items_state;
	matching_init(&ctx);

	matching_feed(&ctx, 'T', true);
	matching_feed(&ctx, 'E', true);
	matching_feed(&ctx, 'S', true);
	matching_feed(&ctx, 'T', true);
	matching_feed(&ctx, '\n', true);

	return 0;
}
```