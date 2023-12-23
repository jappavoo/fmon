CFLAGS=-O2

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

fmon: fmon.o
	${CC} ${CFLAGS} -o $@ $^
