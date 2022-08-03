STATIC_LINKING := 0
AR             := ar
COMMON_FLZ :=  libcommon

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
endif

TARGET_NAME := test
LIBM		= -lm

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else
   ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=link.T
	LIBM :=
else
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -Wl,--version-script=link.T -Wl,--no-undefined
endif

OBJOUT   = -o 
LINKOUT  = -o 
LD = $(CXX)

LDFLAGS += $(SHARED) $(LIBM)
CFLAGS += -O0 -g -DDEBUG
CXXFLAGS += -O0 -g -DDEBUG


include Makefile.common

OBJECTS := $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o) 

CFLAGS += $(INCFLAGS) -Wall -pedantic $(fpic)
CXXFLAGS += $(INCFLAGS) -Wall -pedantic $(fpic)

ifneq (,$(findstring qnx,$(platform)))
CFLAGS += -Wc,-std=c99
else
CFLAGS += -std=gnu99
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS) $(LDFLAGS)
else
	$(LD) $(LINKOUT)$@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< $(OBJOUT)$@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< $(OBJOUT)$@

clean:
	rm -f $(OBJECTS) $(TARGET) $(OBJECTS:.o=.d)

.PHONY: clean

