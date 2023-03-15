client: client.o client_lib.o io_lib.o common_lib.o tcpsocketlib.o compression_lib.o
	gcc -o client client.o client_lib.o common_lib.o tcpsocketlib.o io_lib.o compression_lib.o -lncurses -lz

client.o: client.c client_lib.h io_lib.h ../common/common_lib.h ../common/tcpsocketlib.h  ../common/compression_lib.h
	gcc -c client.c

client_lib.o: client_lib.c client_lib.h io_lib.h ../common/common_lib.h ../common/tcpsocketlib.h  ../common/compression_lib.h
	gcc -c client_lib.c

io_lib.o: io_lib.c io_lib.h
	gcc -c io_lib.c

common_lib.o: ../common/common_lib.c ../common/common_lib.h ../common/tcpsocketlib.h 
	gcc -c ../common/common_lib.c

tcpsocketlib.o: ../common/tcpsocketlib.c ../common/tcpsocketlib.h
	gcc -c ../common/tcpsocketlib.c

compression_lib.o: ../common/compression_lib.c ../common/compression_lib.h
	gcc -c ../common/compression_lib.c

clean: 
	rm -f client client.o client_lib.o io_lib.o common_lib.o tcpsocketlib.o compression_lib.o
