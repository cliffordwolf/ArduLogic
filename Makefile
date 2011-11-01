
CXXFLAGS += -MD -Wall -Os -ggdb
CXX = g++

LDLIBS += -lstdc++ -lm

ardulogic: ardulogic.o parser.o lexer.o genfirmware.o readdata.o writevcd.o \
		decode_jtag.o

parser.cc parser.hh: parser.y
	bison -o parser.cc -d parser.y

lexer.cc: lexer.l
	flex -o lexer.cc lexer.l

clean:
	rm -f ardulogic *.d *.o core
	rm -f parser.cc parser.hh lexer.cc

-include *.d

