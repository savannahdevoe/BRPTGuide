COMPILER=g++
ASSEMBLER=as
LINKER=g++
ARCHIVER=ar

COPTS=-c
AOPTS=
LOPTS=-static

INCLUDE=.
SOURCE=.
OBJECTS=.
LIB=..

vpath %.c $(SOURCE)
vpath %.h $(INCLUDE)
vpath %.s $(SOURCE)
vpath %.i $(INCLUDE)
vpath %.o $(OBJECTS)
vpath %.a $(LIB)

.suffixes: .exe .o .cpp .s
%o: %c
	$(COMPILER) -o$(OBJECTS)/$@ -I $(INCLUDE) $(COPTS) $<
%o: %cpp
	$(COMPILER) -o$(OBJECTS)/$@ -I $(INCLUDE) $(COPTS) $<
%o: %s
	$(ASSEMBLER) -o$(OBJECTS)/$@ -I $(INCLUDE) $(AOPTS) $<

sttp.exe: sttp.o anyoption.o liblua.a
	$(COMPILER) -o $@ $(LOPTS) $^

.PHONY: clean
clean::
	-rm -f *.bak
	-rm -f $(OBJECTS)/*.o

.PHONY: purge
purge: clean
	-rm -f *.exe

