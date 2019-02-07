.POSIX:

CC = cc
CFLAGS = -g -Wall -Wpedantic -Wextra -O0
LDLIBS = -lm
LDFLAGS = -g
PREFIX = /usr/local

all: svsh

svsh: main.o fs/files.o sh/shell.o
	$(CC) $(LDFLAGS) main.o fs/files.o sh/shell.o kerrors.h config.h conversions.h -o svsh_dev $(LDLIBS)

main.o: main.c config.h conversions.h

sh/shell.o: sh/shell.c sh/shell.h

fs/files.o: fs/files.c fs/files.h

kerrors.o: kerrors.c kerrors.h

clean:
	rm *.o */*.o || true
	rm *.gch || true

fullclean:
	rm *.o */*.o || true
	rm *.gch || true
	rm svsh_dev || true
