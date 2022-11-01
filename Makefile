OBJS	= main.o server.o
SOURCE	= main.c server.c
HEADER	= server.h client.h
OUT	= server
FLAGS	 = -ansi -D_POSIX_C_SOURCE=200112L -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -O3 -g

all: server

server: $(OBJS)
	$(CC) -o $@ $^ $(FLAGS)

%.o: %.c $(HEADER)
	$(CC) -c -o $@ $< $(FLAGS)

# clean house
clean:
	rm -f $(OBJS) $(OUT)

# run the program
run: $(OUT)
	./$(OUT)