# Makefile 
CC	     = gcc -std=c99
CXX	     = g++ -std=c++11

CFLAGS   = -Wall -g
CXXFLAGS = -Wall -g

HEADERS = card.h cardset.h dealer.h gamestatus.h player.h \
          simpleplayer.h defaultplayer.h thinkTA1.h 
OBJS = card.o cardset.cpp dealer.o player.o simpleplayer.o defaultplayer.o

.SUFFIXES: .o .cc .cpp .c++ .c

#.c.o: $(HEADERS)
#	$(CC) -c $(CFLAGS) $<
#
#.cc.o: $(HEADERS)
#	$(CXX) -c $(CXXFLAGS) $<
#
#.cpp.o: $(HEADERS)
#	$(CXX) -c $(CXXFLAGS) $<

all: daifugou
	@echo "build finished."

daifugou: daifugou.o $(OBJS) $(HEADERS) libplayingcards.a
	$(CXX) daifugou.o $(OBJS) -o daifugou -L. -lplayingcards

lib: libplayingcards.a

#libplayingcards.a: card.o cardset.o thinkTA1.o
#	ar rcs libplayingcards.a card.o cardset.o thinkTA1.o

depend:
	makedepend *.cpp

clean   :
	/bin/rm -f core daifugou *.o *~
#	rm -f core daifugou `echo *.o | sed -e 's/ThinkTA1.o//'` *~

# DO NOT DELETE

#card.o: /usr/include/stdlib.h /usr/include/bits/libc-header-start.h
#card.o: /usr/include/features.h /usr/include/stdc-predef.h
#card.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
#card.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
#card.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
#card.o: /usr/include/bits/floatn.h /usr/include/bits/floatn-common.h
#card.o: /usr/include/sys/types.h /usr/include/bits/types.h
#card.o: /usr/include/bits/typesizes.h /usr/include/bits/types/clock_t.h
#card.o: /usr/include/bits/types/clockid_t.h /usr/include/bits/types/time_t.h
#card.o: /usr/include/bits/types/timer_t.h /usr/include/bits/stdint-intn.h
#card.o: /usr/include/endian.h /usr/include/bits/endian.h
##card.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
#card.o: /usr/include/bits/uintn-identity.h /usr/include/sys/select.h
#card.o: /usr/include/bits/select.h /usr/include/bits/types/sigset_t.h
#card.o: /usr/include/bits/types/__sigset_t.h
#card.o: /usr/include/bits/types/struct_timeval.h
#card.o: /usr/include/bits/types/struct_timespec.h
#card.o: /usr/include/sys/sysmacros.h /usr/include/bits/sysmacros.h
#card.o: /usr/include/bits/pthreadtypes.h
#card.o: /usr/include/bits/thread-shared-types.h
#card.o: /usr/include/bits/pthreadtypes-arch.h /usr/include/alloca.h
#card.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
#card.o: /usr/include/bits/types/locale_t.h
#card.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h card.h
#cardset.o: card.h cardset.h
#daifugou.o: /usr/include/stdio.h /usr/include/bits/libc-header-start.h
#daifugou.o: /usr/include/features.h /usr/include/stdc-predef.h
#daifugou.o: /usr/include/sys/cdefs.h /usr/include/bits/wordsize.h
#daifugou.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
#daifugou.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
#daifugou.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
##daifugou.o: /usr/include/bits/libio.h /usr/include/bits/_G_config.h
#daifugou.o: /usr/include/bits/types/__mbstate_t.h
##daifugou.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
#daifugou.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
#daifugou.o: /usr/include/bits/waitstatus.h /usr/include/bits/floatn.h
#daifugou.o: /usr/include/bits/floatn-common.h /usr/include/sys/types.h
#daifugou.o: /usr/include/bits/types/clock_t.h
#daifugou.o: /usr/include/bits/types/clockid_t.h
#daifugou.o: /usr/include/bits/types/time_t.h
#daifugou.o: /usr/include/bits/types/timer_t.h /usr/include/bits/stdint-intn.h
#daifugou.o: /usr/include/endian.h /usr/include/bits/endian.h
##daifugou.o: /usr/include/bits/byteswap.h /usr/include/bits/byteswap-16.h
#daifugou.o: /usr/include/bits/uintn-identity.h /usr/include/sys/select.h
#daifugou.o: /usr/include/bits/select.h /usr/include/bits/types/sigset_t.h
#daifugou.o: /usr/include/bits/types/__sigset_t.h
#daifugou.o: /usr/include/bits/types/struct_timeval.h
#daifugou.o: /usr/include/bits/types/struct_timespec.h
#daifugou.o: /usr/include/sys/sysmacros.h /usr/include/bits/sysmacros.h
#daifugou.o: /usr/include/bits/pthreadtypes.h
#daifugou.o: /usr/include/bits/thread-shared-types.h
#daifugou.o: /usr/include/bits/pthreadtypes-arch.h /usr/include/alloca.h
#daifugou.o: /usr/include/bits/stdlib-float.h /usr/include/getopt.h
#daifugou.o: /usr/include/bits/getopt_core.h /usr/include/bits/getopt_ext.h
#daifugou.o: card.h cardset.h player.h gamestatus.h simpleplayer.h
#daifugou.o: defaultplayer.h thinkTA1.h dealer.h
dealer.o: card.h cardset.h player.h gamestatus.h dealer.h
defaultplayer.o: defaultplayer.h player.h gamestatus.h card.h cardset.h
player.o: card.h cardset.h player.h gamestatus.h
simpleplayer.o: card.h cardset.h player.h gamestatus.h simpleplayer.h
