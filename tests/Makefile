
CXX = g++
CXXFLAGS = -Wall

all: data_spi.raw

data_spi.raw: gendata_spi
	./gendata_spi > data_spi.new
	mv data_spi.new data_spi.raw

clean:
	rm -f data_spi.raw gendata_spi

