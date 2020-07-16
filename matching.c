#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "zringbuf.h"
#include "zabuffer.h"
#include "matching.h"

/**
 * Run match callback when flags match
 */
static bool matching_item_cb(struct matching_ctx *ctx, uint8_t flags)
{
	enum matching_item_flags _flags = MATCHING_ITEM_FLAGS_DEFAULTS;

	if (ctx->match.item == NULL || ctx->match.item->cb == NULL) {
		return false;
	}

	if (ctx->match.item->flags == MATCHING_ITEM_FLAGS_DEFAULTS) {
		_flags = ctx->cfg.flags;
	} else {
		_flags = ctx->match.item->flags;
	}

	if (!(_flags & flags)) {
		return false;
	}

	ctx->match.item->cb(ctx);

	return true;
}

/**
 * Try matching on item(s)
 * - When ctx->match is set, run callback
 * - Itterate over all ctx->items and verify match
 * - ctx->match is set to item when item string full matched
 */
static void matching_match(struct matching_ctx *ctx, char ch)
{
	for (size_t n = 0; n < ctx->items.n; n++) {
		enum matching_item_states *st = &ctx->items.state[n];

		if (*st == MATCHING_STATE_UNMATCHABLE || *st == MATCHING_STATE_MATCHED) {
			continue;
		}

		const struct matching_item *s = &ctx->items.list[n];

		/* Check current position exceeds the size of the current item */
		if (ctx->match.pos >= s->size) {
			*st = MATCHING_STATE_UNMATCHABLE;
			continue;
		}

		/* Check character doesn't match the position in the current item */
		if (ch != s->s[ctx->match.pos]) {
			*st = MATCHING_STATE_UNMATCHABLE;
			continue;
		}

		/* Check if item is not fully matched */
		if (ctx->match.pos != (s->size - 1)) {
			*st = MATCHING_STATE_CAN_MATCH;
			continue;
		}

		ctx->match.str = (char *)ctx->linebuffer.cur;
		*st = MATCHING_STATE_MATCHED;
		ctx->match.item = s;
		matching_item_cb(ctx, MATCHING_ITEM_FLAG_CB_ON_MATCH);
	}
}

void matching_decode(struct matching_ctx *ctx)
{
	char ch;
	if (zringbuf_is_empty(&ctx->ringbuf)) {
		return;
	}
	zringbuf_dequeue(&ctx->ringbuf, &ch);
	za_buffer_write_u8(&ctx->linebuffer, ch);
	if (ctx->cc) {
		za_buffer_write_u8(ctx->cc, ch);
	}

	if (ctx->skip) {
		ctx->skip--;
	} else {
		/* Reset match engine when reset char is seen */
		if (NULL != strchr(ctx->cfg.reset_chars, ch)) {
			matching_reset(ctx);
			return;
		}

		matching_match(ctx, ch);
		ctx->match.pos++;
	}
}

void matching_skip(struct matching_ctx *ctx, uint32_t skip)
{
	ctx->skip = skip;
}

void matching_init(struct matching_ctx *ctx)
{
	ctx->ringbuf.size = MATCHING_RINGBUF_SIZE;
	ctx->ringbuf.buffer = ctx->ringbuf_data;
	zringbuf_init(&ctx->ringbuf);
	matching_reset(ctx);
}

void matching_reset(struct matching_ctx *ctx)
{
	matching_item_cb(ctx, MATCHING_ITEM_FLAG_CB_ON_RESET);

	memset(ctx->items.state, MATCHING_STATE_NO_MATCH, sizeof(ctx->items.state[0]) * ctx->items.n);
	memset(&ctx->match, 0, sizeof(ctx->match));
	za_buffer_flush(&ctx->linebuffer);
}

void matching_feed(struct matching_ctx *ctx, char c, bool decode_now)
{
	if (ctx == NULL) {
		return;
	}
	zringbuf_queue(&ctx->ringbuf, c);
	if (decode_now) {
		matching_decode(ctx);
	}
}
