server: server.o server_lib.o common_lib.o tcpsocketlib.o compression_lib.o
	gcc -o server server.o server_lib.o common_lib.o tcpsocketlib.o compression_lib.o -lz

server.o: server.c server_lib.h ../common/common_lib.h ../common/tcpsocketlib.h  ../common/compression_lib.h
	gcc -c server.c

server_lib.o: server_lib.c server_lib.h ../common/common_lib.h ../common/tcpsocketlib.h  ../common/compression_lib.h
	gcc -c server_lib.c

common_lib.o: ../common/common_lib.c ../common/common_lib.h ../common/tcpsocketlib.h 
	gcc -c ../common/common_lib.c

tcpsocketlib.o: ../common/tcpsocketlib.c ../common/tcpsocketlib.h
	gcc -c ../common/tcpsocketlib.c

compression_lib.o: ../common/compression_lib.c ../common/compression_lib.h
	gcc -c ../common/compression_lib.c

clean: 
	rm -f server server.o server_lib.o common_lib.o tcpsocketlib.o compression_lib.o