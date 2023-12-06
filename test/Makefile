CFLAGS?=-Wall -std=c90
TEST_LZ77?=./test_lz77

all: test_lz77

test_lz77: test_lz77.c ../src/lz77.c
	@$(CC) -o $(TEST_LZ77)  $(CFLAGS) -I../include ../src/lz77.c ./test_lz77.c

clean :
	@$(RM) $(TEST_LZ77)
