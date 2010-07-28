LIBLIST=glib-2.0

LIBREF=`pkg-config --libs ${LIBLIST}`
CFLAGREF=`pkg-config --cflags ${LIBLIST}`
DEBUG=-Wall

all:
	gcc ${DEBUG} -c ms_enc.c ${CFLAGREF}
	gcc ${DEBUG} -o ms_enc ms_enc.o ${LIBREF}

clean:
	rm -rf ms_enc ms_enc.o
