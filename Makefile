TARGETS=fs-walker gendust

all: $(TARGETS)

fs-walker: fs-walker.c
	$(CC) -ggdb $^ -o $@

gendust: gendust.cpp
	$(CXX) -ldevmapper -ggdb $^ -o $@

clean:
	rm -f $(TARGETS)
