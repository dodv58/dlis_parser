.PHONY: all clean
OUTDIR:=output
APP:=dlis-parse
S_DIR:=src

SRCS:=common.c dlis.c main.c

all: $(APP)

testapp: test.o src/common.o
	gcc -o $@ $^
test.o: test.c
	gcc -c test.c
$(APP): $(addprefix $(S_DIR)/, $(SRCS:.c=.o))
	gcc -o $@ $^

$(S_DIR)/%.o: $(S_DIR)/%.c
	gcc -c -o $@ $<

clean:
	rm -fr $(APP) $(S_DIR)/*.o test.o testapp
