.PHONY: all clean
OUTDIR:=output
APP:=sayhello
S_DIR:=src

SRCS:=dlis.c main.c

all: $(APP)

$(APP): $(addprefix $(S_DIR)/, $(SRCS:.c=.o))
	gcc -o $@ $^

$(S_DIR)/%.o: $(S_DIR)/%.c
	gcc -c -o $@ $<

clean:
	rm -fr $(APP) $(S_DIR)/*.o
