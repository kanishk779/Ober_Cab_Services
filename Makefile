output: Ober_services.o
	gcc Ober_services.o -o output
Ober_services.o: Ober_services.c Ober_services.h
	gcc -c Ober_services.c
clean:
	rm output *.o
	
