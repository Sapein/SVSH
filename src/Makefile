.POSIX:

CC = cc
CFLAGS = -g -Wall -Wpedantic -Wextra -O0
LDLIBS = -lm
LDFLAGS = -g
PREFIX = /usr/local

all: svsh

svsh: main.o fs/files.o
	$(CC) $(LDFLAGS) main.o fs/files.o config.h -o svsh_dev $(LDLIBS)

main.o: main.c config.h

fs/files.o: fs/files.c fs/files.h

clean:
	rm *.o */*.o || true
	rm *.gch || true

fullclean:
	rm *.o */*.o || true
	rm *.gch || true
	rm svsh_dev || true

