
all: dining_philosophers waiter philosopher

dining_philosophers: dining_philosophers.o waiter philosopher
	gcc $< -g -o $@

waiter: waiter.o
	gcc $< -g -o $@
	
philosopher: philosopher.o
	gcc $< -g -o $@
	
dining_philosophers.o: dining_philosophers.c clean_tmp_fifo
	gcc -c $<
	
waiter.o: waiter.c
	gcc -c $<

philosopher.o: philosopher.c
	gcc -c $<

.PHONY: clean

clean:
	rm -f dining_philosophers.o
	rm -f waiter.o
	rm -f philosopher.o