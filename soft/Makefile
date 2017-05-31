
#OS = LINUX
OS = MACOSX
#OS = WINDOWS

PROG = SMS_dumper

# To set up Ubuntu Linux to cross compile for Windows:
# apt-get install mingw32 mingw32-binutils mingw32-runtime
# Just edit the variable above for WINDOWS, then use "make" to build rawhid.exe

ifeq ($(OS), LINUX)
TARGET = $(PROG)
CC = gcc
STRIP = strip
CFLAGS = -Wall -O2 -DOS_$(OS) -std=c99
LIBS = -lusb
else ifeq ($(OS), MACOSX)
TARGET = $(PROG)
SDK = /Users/fabrice/Developer/SDKs/MacOSX10.9.sdk
ARCH = -mmacosx-version-min=10.9 -arch i386
CC = gcc
STRIP = strip
CFLAGS = -Wall -O2 -DOS_$(OS) -isysroot $(SDK) $(ARCH)
LIBS = $(ARCH) -Wl,-syslibroot,$(SDK) -framework IOKit -framework CoreFoundation
else ifeq ($(OS), WINDOWS)
TARGET = $(PROG).exe
#CC = i586-mingw32msvc-gcc
CC = gcc
STRIP = strip
CFLAGS = -Wall -O2 -DOS_$(OS)
LIBS = -lhid -lsetupapi
endif

OBJS = $(PROG).o hid.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(LIBS)
	$(STRIP) $(TARGET)

hid.o: hid_$(OS).c hid.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
ifeq ($(OS), WINDOWS)
	del *.o
	del $(TARGET)
else 
	rm -f *.o $(TARGET)
endif
