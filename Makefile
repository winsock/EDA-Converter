IDIR=include
CC=clang
CXX=clang++
CFLAGS=-I$(IDIR)
CXXFLAGS=$(CFLAGS) -std=c++11

OUTNAME=converter
OUTDIR=bin

ODIR=obj
LDIR=lib

LIBS=

_DEPS =
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = converter.o openjson.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

all: $(OBJ)
	$(CXX) -o $(OUTDIR)/$(OUTNAME) $^ $(CXXFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 