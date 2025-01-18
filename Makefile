barrier: barrier.cpp Makefile
	clang++ -stdlib=libc++ --std=c++20 -Wall -Wextra -pedantic -Werror -o $@ -fsanitize=thread -g -Og $<
