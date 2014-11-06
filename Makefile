CFLAGS= -Wunused-variable

all: ba63gui

ba63gui: ba63gui.c
	gcc $(CFLAGS) -o ba63gui ba63gui.c `pkg-config --cflags --libs gtk+-3.0`
