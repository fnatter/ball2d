ROOT = /djgpp/fx
BASH = /bin/sh # needed for a few targets (but not the default build targets)
INCLDIR = ../incl
NASM = nasm # consider using nasmw if you`re running windows
LD = gpp
CXX = gpp
GLOBAL_ETAGS_FILES = *.cpp *.h *.asm *.c
# can't use -pedantic because vbetwo.cpp uses "long long"
GLOBAL_CXXFLAGS = -Wall
GLOBAL_RELEASE_CXXFLAGS = $(GLOBAL_CXXFLAGS) -DDJ_RELEASE -O6 -fstrict-aliasing
GLOBAL_DEBUG_CXXFLAGS = $(GLOBAL_CXXFLAGS) -gstabs+
RELEASE_FLAG = -DDJ_RELEASE
DEBUG_MSG = You have compiled a debug version. Uncomment RELEASE= in ./Makefile \
to build an optimized RELEASE-version
