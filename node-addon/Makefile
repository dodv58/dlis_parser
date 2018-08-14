.PHONY: all clean
OUTDIR:=output
APP:=dlis-parse
S_DIR:=src

CFLAGS:=-g

SRCS:=common.cc dlis.cc main.cc

all: $(APP)

testapp: test.o src/common.o
	g++ -o $@ $^
test.o: test.cc
	g++ -c test.cc $(CFLAGS)
$(APP): $(addprefix $(S_DIR)/, $(SRCS:.cc=.o))
	g++ -o $@ $^

$(S_DIR)/%.o: $(S_DIR)/%.cc
	g++ -c -o $@ $< $(CFLAGS)

clean:
	rm -fr $(APP) $(S_DIR)/*.o test.o testapp *.o a.out
