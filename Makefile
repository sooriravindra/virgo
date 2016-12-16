
SRCS=virgo.cpp
OBJS=$(SRCS:.cpp=.o)
CFLAGS=-std=c89 -pedantic -Wall -O3 -nostdlib -fno-asynchronous-unwind-tables -fpermissive -fno-builtin -fno-ident -ffunction-sections -fdata-sections
LIBS=-lgdi32 -luser32 -lshell32 -lkernel32 -lmsvcrt 
LDFLAGS=-static -nostdlib -fno-builtin -s -Wl,-e,__main,--gc-sections,-subsystem,windows $(LIBS)
ARCH=32
CC=g++
ifeq ($(ARCH), 64)
	WINDRES_ARCH=pe-x86-64
else
	WINDRES_ARCH=pe-i386
endif
NAME=virgo
EXE=$(NAME).exe

.PHONY: all clean
all: $(EXE)
$(EXE): $(OBJS) $(NAME).res
	$(CC) -o $(EXE) $(OBJS) $(NAME).res -m$(ARCH) $(LDFLAGS)
	
$(NAME).res: $(NAME).rc
	windres -O coff -F $(WINDRES_ARCH) $(NAME).rc $(NAME).res 
	
.cpp.o:
	$(CC) -o $@ $(CFLAGS) -m$(ARCH) -c $<

clean:
	rm -f $(OBJS) $(EXE) $(NAME).res
	
format:
	astyle --mode=c --style=kr --indent=tab --pad-header --delete-empty-lines --align-pointer=name --max-code-length=80 virgo.cpp
