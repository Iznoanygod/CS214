all:
	gcc heapSort.c -g -c -o heapSort.o
	gcc fileCompressor.c -g -o fileCompressor heapSort.o
rtest:
	rm ctest/*.hcz
	rm dtest/*.txt
clean:
	rm fileCompressor
