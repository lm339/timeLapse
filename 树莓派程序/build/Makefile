CFLAGS = -c -Wall
CC = gcc

OBJS_KEY = main_key.o updateImgToserver.o

VPATH = include

DEBUG =  `pkg-config --cflags --libs glib-2.0`  -lpthread -ldl -lwiringPi


vpath %.c ../
vpath %.c ../Key

all: main_key

${OBJS_KEY}: %.o: %.c
	${CC} -std=c99 ${CFLAGS} ${DEBUG} -c $< 

main_key: ${OBJS_KEY}
	${CC} ${OBJS_KEY} -o $@ ${DEBUG} 


.PHONY: clean
clean:
	rm -rf *o main main_key



