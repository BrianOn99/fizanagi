vpath %.c src
vpath %.h src

name = fizanagi 

CPPFLAGS = -g --std=gnu99 -I src

$(name): main.c common.o fatinfo.o
	gcc $(CPPFLAGS) $^ -o $@

common.o: common.h

fatinfo.o: fatinfo.h

.PHONY: clean

clean:
	@rm *.o $(name) 2> /dev/null || true
