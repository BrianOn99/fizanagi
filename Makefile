vpath %.c src
vpath %.h src

name = fizanagi 

CPPFLAGS = -g -std=c99 -I src

$(name): CPPFLAGS += -D_DEBUG
$(name): main.c common.o fatinfo.o readcluster.o mylfn.o
	gcc $(CPPFLAGS) $^ -o $@

dumb: CPPFLAGS += -DDUMBMODE
dumb: main.c common.o fatinfo.o readcluster.o mylfn.o
	gcc $(CPPFLAGS) $^ -o $(addsuffix _dumb, $(name))

common.o: common.h

fatinfo.o: fatinfo.h

readcluster.o: readcluster.h

mylfn.o: mylfn.h

.PHONY: clean

clean:
	@rm *.o $(name) $(addsuffix _dumb, $(name)) 2> /dev/null || true
