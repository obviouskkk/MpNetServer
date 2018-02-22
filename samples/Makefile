CC=g++

LD_FLAGS= -L /home/zzy/MpNetServer/iniparser -liniparser 
INCLUDE_FLAGS=-I/usr/include/glib-2.0/  -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ \
			  -I/home/zzy/MpNetServer/iniparser/src
CFLAGS=-Wall -fPIC  -ggdb  

TARGET = echo.so echo_client
all:$(TARGET)

echo.so:echo.o
	$(CC) -shared  -o $@ $^
echo_client:echo_client.o
	$(CC)  -o $@ $^ $(LD_FLAGS)

.cpp.o:
	$(CC)  $(INCLUDE_FLAGS) $(CFLAGS) -c $<  

clean:
	@rm -rf *.o $(TARGET) log
