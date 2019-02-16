linker: linker.cpp
	module load gcc-4.9.2; \
	g++ -std=c++11 linker.cpp -o linker; \
	module unload gcc-4.9.2; \
	module list

clean:
	rm -f linker *.o