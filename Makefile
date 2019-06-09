module: redismodule.h
	gcc -fpic -O2 -Wall -Werror -Wextra -pedantic -shared -march=native -o fencelock.so fencelock.c

.PHONY: fmt
fmt:
	\indent fencelock.c

.PHONY: clean
clean:
	\rm -f fencelock.so
