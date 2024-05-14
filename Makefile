FLAGS=-fopenmp -lnuma

all: clean
	$(CXX) -O2 bench.cc -o bench $(FLAGS)
	$(CC) -O2 nmstat.c -o nmstat 2> /dev/null

clean:
	$(RM) bench nmstat
