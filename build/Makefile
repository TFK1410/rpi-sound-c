LDFLAGS = -lportaudio -lpthread -lm -lfftw3 -lstdc++
CFLAGS = -Wall -O3 -I.
SOURCES = ../src/main.c ../src/loops.c ../src/utils.c ../src/configuration.c ../inih/ini.c
NAME = fft-c

RGB_LIB_DISTRIBUTION=../rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt
CFLAGS+=-I$(RGB_INCDIR)

OBJECTS = $(SOURCES:.c=.o)

all: $(NAME)

$(NAME): $(OBJECTS)
		$(CC) $(OBJECTS) $(LDFLAGS) -o $(NAME)

%.o: %.c
		$(CC) -c $(CFLAGS) $< -o $@

 $(RGB_LIBRARY):
	    $(MAKE) -C $(RGB_LIBDIR)

clean:
		rm -f $(OBJECTS)
		rm -f $(NAME)
		rm -f raw_wav
