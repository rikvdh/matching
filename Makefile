
test: matching.c test.c deps/zringbuf/zringbuf.c deps/zabuffer/zabuffer.c
	@$(CC) $^ -std=c99 -Ideps/zringbuf -Ideps/zabuffer -Wall -pedantic -Werror -o $@
	@./test

lint: checkpatch.pl
	./checkpatch.pl --no-tree -f --terse --mailback \
		--max-line-length=120 \
		matching.c test.c test.h

checkpatch.pl:
	curl https://raw.githubusercontent.com/libopencm3/libopencm3/master/scripts/checkpatch.pl > $@
	chmod +x $@

.PHONY: test lint
