
C++ = g++

INCLUDE = -Iinclude/libhls/include \
	-Iinclude/libflv/include \
	-Iinclude/libmov/include \
	-Iinclude/libmpeg/include 

LIBPATHS = -Llib/

CCFLAGS = -fPIC -Wall -g3

OBJS = hls-ts-test.o \
	hls-fmp4-test.o

TARGET = hls-ts-test

.PHONY : all clean
all: $(TARGET)

$(OBJS) : %.o : %.cpp
	$(C++) -c $(CCFLAGS) $(INCLUDE) $< -o $@

$(TARGET): $(OBJS)
	g++ -o hls-ts-test hls-ts-test.o $(LIBPATHS) -lflv -lhls -lmov -lmpeg
	g++ -o hls-fmp4-test hls-fmp4-test.o $(LIBPATHS) -lflv -lhls -lmov -lmpeg

clean:
	rm -f $(OBJS) $(TARGET) 
