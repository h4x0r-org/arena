.PHONY: clean
COMPILE_FLAGS=-Wall -g -I./include -ffast-math $(CFLAGS)
LINK_FLAGS=-flto $(LDFLAGS)
obj_files = alloc.o test.o

all: $(obj_files)
	$(CC) $(LINK_FLAGS) -o alloc_test $(obj_files)

clean:
	rm alloc_test *.o

$(obj_files): %.o: %.c
	$(CC) -c $(COMPILE_FLAGS) $< -o $@


alloc.o: alloc.c include/h4x0r/alloc.h include/h4x0r/random.h
test.o: test.c include/h4x0r/alloc.h include/h4x0r/random.h include/h4x0r/timing.h
