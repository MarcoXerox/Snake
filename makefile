LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
CC = "clang++"

main: main.cpp
	$(CC) $(CFLAGS) $(LDLIBS) $? $(LDFLAGS) -o $@
