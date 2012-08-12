OBJ = main.o socks.o work.o 
socks5: $(OBJ)
	g++ $(OBJ) -lpthread -o socks5
main.o: main.cpp work.h
	g++ -c main.cpp
socks.o: socks.cpp socks.h util.h
	g++ -c socks.cpp
work.o: work.cpp socks.h
	g++ -c work.cpp
clean: 
	rm -f *.o
