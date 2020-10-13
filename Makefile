NAME=maedn
CFLAGS=-g -ggdb -Wall -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable -o $(NAME)
GTKFLAGS=-export-dynamic `pkg-config --cflags --libs gtk+-2.0 cairo libcanberra`
SRCS=maedn.c
CC=gcc

# top-level rule to create the program
all: tetris

# compiling the source file
tetris: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(GTKFLAGS)

#	$(CC) $(CFLAGS) -m32 -o $(NAME)_32 $(SRCS) $(GTKFLAGS)



# cleaning everything that can be automatically recreated with "make"
clean:
	/bin/rm -f $(NAME)
