TARGETS=fs-walker gendust

all: $(TARGETS)

fs-walker: fs-walker.c
	$(CC) -ggdb $^ -o $@

gendust: gendust.c
	$(CC) -ldevmapper -ggdb $^ -o $@

clean:
	rm -f $(TARGETS)
