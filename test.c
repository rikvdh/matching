#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "matching.h"

#define LOG_TEST_START() printf("%s\n", __FUNCTION__)

static struct matching_ctx ctx;
static char ctx_linebuffer[256];
static unsigned int match_count;
static unsigned int match_count_ko;
static const char input_data[] = "\r\nOK\r\nKO\r\nOK\r\nKO\r\nOK\r\nHI\r\n";

static void matching_cb_match(struct matching_ctx *ctx)
{
	(void)ctx;
	match_count++;
}

static void matching_cb_match_ko(struct matching_ctx *ctx)
{
	(void)ctx;
	match_count_ko++;
}

static void matching_cb_match_rickroll(struct matching_ctx *ctx)
{
	(void)ctx;
	assert(0 == strcmp("gonna give you up\r\n", ctx->match.str));
	assert(0 == strcmp("+NEVER:gonna give you up\r\n", (const char *)ctx->linebuffer.data));
}

static void matching_cb_nsmi(struct matching_ctx *ctx)
{
	(void)ctx;
	assert(0 == strcmp("SENT\r\n", ctx->match.str));
}

static const struct matching_item matching_items[] = {
	{ MATCHING_ITEM_STR("HI"), 0, NULL },
	{ MATCHING_ITEM_STR("OK"), 0, matching_cb_match },
	{ MATCHING_ITEM_STR("KO"), MATCHING_ITEM_FLAG_CB_ON_MATCH | MATCHING_ITEM_FLAG_CB_ON_RESET, matching_cb_match_ko },
	{ MATCHING_ITEM_STR("+NEVER:"), 0, matching_cb_match_rickroll },
	{ (const char *)"+NSMI:\0ERROR", 6, 0, matching_cb_nsmi }
};

static enum matching_item_states matching_items_state[sizeof(matching_items)/sizeof(matching_items[0])];

void prepare_ctx(void)
{
	memset(&ctx, 0, sizeof(struct matching_ctx));

	match_count = 0;
	match_count_ko = 0;

	ctx.cfg.flags           = MATCHING_ITEM_FLAG_CB_ON_RESET;
	ctx.cfg.reset_chars     = "\n";

	za_buffer_init(&ctx.linebuffer, ctx_linebuffer, sizeof(ctx_linebuffer));

	ctx.items.list          = matching_items;
	ctx.items.n             = sizeof(matching_items)/sizeof(matching_items[0]);
	ctx.items.state         = matching_items_state;
	matching_reset(&ctx);
}

void test_matching_feed_invalid(void)
{
	LOG_TEST_START();
	matching_feed(NULL, input_data[0], true);
}

/* Count amount of callbacks calls */
void test_matching_count_cb_calls(void)
{
	LOG_TEST_START();
	prepare_ctx();

	for (int i = 0; i < strlen(input_data); i++) {
		matching_feed(&ctx, input_data[i], true);
	}

	assert(3U == match_count);
	assert(4U == match_count_ko);
}

/* Feed more than the linebuffer without match, check for crash */
void test_matching_check_linebuffer_overflow(void)
{
	prepare_ctx();

	for (int i = 0; i < sizeof(ctx_linebuffer) + 1; i++) {
		matching_feed(&ctx, 0x55, true);
	}

	/* Expect we are at the end of the linebuffer with matching */
	assert(ctx.linebuffer.used == ctx.linebuffer.size - 1);
}

/* No linebuffer, check if we match */
void test_matching_linebuffer_null(void)
{
	prepare_ctx();
	ctx.linebuffer.data = NULL;

	for (int i = 0; i < strlen(input_data); i++) {
		matching_feed(&ctx, input_data[i], true);
	}

	assert(3U == match_count);
	assert(4U == match_count_ko);
}

/* Count amount of callbacks calls */
void test_matching_multi_chunk_ok_test(void)
{
	LOG_TEST_START();
	const char chunk1[] = "Lorem ipsum, lorem ipsum\r\nO";
	const char chunk2[] = "K\r\nmust match..\r\n";

	prepare_ctx();

	for (int i = 0; i < strlen(chunk1); i++) {
		matching_feed(&ctx, chunk1[i], true);
	}
	for (int i = 0; i < strlen(chunk2); i++) {
		matching_feed(&ctx, chunk2[i], true);
	}

	assert(1U == match_count);
	assert(0U == match_count_ko);
}

void test_matching_rickroll(void)
{
	LOG_TEST_START();
	const char chunk1[] = "\r\n\r\n+NEVER";
	const char chunk2[] = ":gonna give you ";
	const char chunk3[] = "up\r\n\r\nKO\r\nLALALA\r\n";

	prepare_ctx();

	assert(0U == match_count);
	assert(0U == match_count_ko);
	for (int i = 0; i < strlen(chunk1); i++) {
		matching_feed(&ctx, chunk1[i], true);
	}
	assert(0U == match_count);
	assert(0U == match_count_ko);
	for (int i = 0; i < strlen(chunk2); i++) {
		matching_feed(&ctx, chunk2[i], true);
	}
	assert(0U == match_count);
	assert(0U == match_count_ko);

	for (int i = 0; i < strlen(chunk3); i++) {
		matching_feed(&ctx, chunk3[i], true);
	}
	assert(0U == match_count);
	assert(2U == match_count_ko);
}

void test_matching_nsmi(void)
{
	LOG_TEST_START();
	const char chunk1[] = "\r\nOK\r\n\r\n+NSMI:SENT\r\n\r\nOK\r\n";

	prepare_ctx();

	for (int i = 0; i < strlen(chunk1); i++) {
		matching_feed(&ctx, chunk1[i], true);
	}
}

int main(void)
{
	test_matching_feed_invalid();
	test_matching_count_cb_calls();
	test_matching_check_linebuffer_overflow();
	test_matching_linebuffer_null();
	test_matching_multi_chunk_ok_test();
	test_matching_rickroll();
	test_matching_nsmi();
}
