CC = g++
#CFLAGS = -I/home/ydong/xmt-git/xmt-externals/Shared/cpp/boost_1_58_0/include -pthread -std=c++11
CFLAGS = -pthread -std=c++11
LIBS = -lstdc++ #-lboost_system-mt
#LFLAGS = -L/home/ydong/xmt-git/xmt-externals/FC12/libraries/boost_1_58_0/lib/

pctest: pctest.cpp
	$(CC) -o pctest $(CFLAGS) $(LFLAGS) $(LIBS) pctest.cpp
#g++ -pthread -std=c++11 -lstdc++ pctest.cpp
clean:
	rm pctest
