CC := gcc
CFLAGS := -Wall -Werror -Wextra -ggdb
TARGET := ../target

all: clean
	mkdir ../target
	$(CC) $(CFLAGS) ls.c -o $(TARGET)/my-ls

clean:
	rm -rf $(TARGET)