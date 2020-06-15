#ifndef MATCHING_H__
#define MATCHING_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "zringbuf.h"
#include "zabuffer.h"

#ifdef __cplusplus
#define MATCHING_ITEM_STR(_s) reinterpret_cast<const uint8_t *>(_s), sizeof(_s) - 1
#else
#define MATCHING_ITEM_STR(_s) .s = (const uint8_t *)_s, .size = sizeof(_s) - 1
#endif

#ifndef MATCHING_RINGBUF_SIZE
#define MATCHING_RINGBUF_SIZE 128
#endif

/**
 * Matching item state
 */
enum matching_item_states {
	MATCHING_STATE_NO_MATCH    = 0, /**< Item not matched */
	MATCHING_STATE_UNMATCHABLE = 1, /**< Item is unmatchable */
	MATCHING_STATE_CAN_MATCH   = 2, /**< Item can match */
	MATCHING_STATE_MATCHED     = 3  /**< Item is matched */
};

/**
 * Matching engine item flags
 */
enum matching_item_flags {
	MATCHING_ITEM_FLAGS_DEFAULTS   = 0,        /**< Default flags are used from matching_ctx.cfg.flags */
	MATCHING_ITEM_FLAG_CB_ON_MATCH = (1 << 0), /**< Execute callback on match */
	MATCHING_ITEM_FLAG_CB_ON_RESET = (1 << 1)  /**< Execute callback on reset */
};

/**
 * Matching engine context
 */
struct matching_ctx {
	char ringbuf_data[MATCHING_RINGBUF_SIZE];
	struct zringbuf ringbuf;
	struct za_buffer linebuffer;

	/** Configuration */
	struct cfg {
		uint8_t flags;              /**< Default item flags */
		const char *reset_chars;    /**< Statemachine reset characters */
		void *private_data;         /**< Private data field for callback context */
	} cfg;

	struct match {
		const struct matching_item *item; /**< Current matched item */
		size_t pos;                    /**< Current position of match */
		char *str;                     /**< Matched item string in linebuffer */
	} match;

	struct items {
		const struct matching_item *list; /**< Items to match */
		enum matching_item_states *state; /**< Items state */
		size_t n;                      /**< Items count */
	} items;
};

/**
 * Matching item
 */
struct matching_item {
	const char *s;         /**< Item string */
	const size_t size;     /**< String size */
	uint8_t flags;         /**< Item flags, when set to MATCHING_ITEM_FLAGS_DEFAULTS.
				* matching_ctx.cfg.flags is used
				*/
	void (*cb)(struct matching_ctx *ctx); /**< Item match callback */
};

/**
 * Decode a single character
 * - Reset statemachine on ctx->cfg.reset_char
 * - Chomp all non printables (less than ' ' || bigger than '~')
 */
void matching_decode(struct matching_ctx *ctx);

/**
 * Initializes the matching engine
 * - Clear ring-buffer
 * - Resets the matching engine via matching_reset
 */
void matching_init(struct matching_ctx *ctx);

/**
 * Reset matching engine
 * - Clear ctx->cfg.linebuffer.data
 * - Set all ctx->item->flags to default when set to MATCHING_ITEM_FLAGS_DEFAULTS
 * - Set all ctx->items state to NOMATCH
 * - Set ctx->pos = 0
 * - Set ctx->match_prev
 * - Set ctx->match = NULL
 */
void matching_reset(struct matching_ctx *ctx);

/**
 * Feed data into the engine
 */
void matching_feed(struct matching_ctx *ctx, char c, bool decode_now);

#ifdef __cplusplus
}
#endif

#endif /* MATCHING_H__ */
