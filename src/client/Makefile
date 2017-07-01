

CC = gcc
LD = gcc

CFLAGS = -g -O3
BIN = test

SOURCES=test_connection.c
OBJECTS=$(SOURCES:.c=.o)


all: ${BIN}

clean:
	rm -f *.ao *.o *.axf ${BIN}

${BIN}: $(OBJECTS)
	${LD} ${LDFLAGS} -o $@ $(OBJECTS)

%.o: %.c
	 ${CC} ${CFLAGS} -c $< -o $@
