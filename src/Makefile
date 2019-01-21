.POSIX:

CC = cc
CFLAGS = -g -H -Wall -Wpedantic -Wextra -O0
LDLIBS = -lm
LDFLAGS = -g -H
PREFIX = /usr/local

all: svsh

svsh: main.o fs/files.o
	$(CC) $(LDFLAGS) main.o fs/files.o config.h -o svsh_dev $(LDLIBS)

main.o: main.c config.h

fs/files.o: fs/files.c fs/files.h

clean:
	rm *.o */*.o || true

fullclean:
	rm *.o */*.o || true
	rm svsh_dev || true

