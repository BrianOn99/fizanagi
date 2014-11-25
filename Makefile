vpath %.c src
vpath %.h src

name = fizanagi 

CPPFLAGS = -g --std=gnu99 -I.

$(name): main.c
	gcc -g $^ -o $@

.PHONY: clean

clean:
	@rm *.o $(name) 2> /dev/null || true
