.PHONY: all clean
SDIR := src
SRCS := utils.c parser.c circular-buffer.c
M_SRCS := main.c
T_SRCS := test.c 
ODIR := build
OBJS := $(SRCS:.c=.o)

INCLUDES :=
LIBS :=

all: dlis

test: testApp

testApp: $(addprefix $(ODIR)/, $(OBJS)) $(ODIR)/$(T_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)

dlis: $(addprefix $(ODIR)/, $(OBJS)) $(ODIR)/$(M_SRCS:.c=.o)
	gcc -o $@ $^ $(LIBS)
	
$(ODIR)/%.o : $(SDIR)/%.c
	gcc -c -o $@ $^ $(INCLUDES)
	
clean:
	rm -fr dlis testApp $(addprefix $(ODIR)/,$(OBJS) $(T_SRCS:.c=.o) $(M_SRCS:.c=.o))
