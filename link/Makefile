SRC=$(wildcard *.c)
OBJ=$(patsubst %.c, %.o, $(SRC))

CFLAGS=-Wall -Wextra -Werror=implicit-function-declaration -Wno-unused-function -g -Iinclude
LDFLAGS=

TARG=dist/link

.PHONY: all clean

all: $(TARG)

$(TARG): $(OBJ)
	@mkdir -p dist
	@echo "LD    $@"
	@$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	@echo "CC    $@"
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm $(TARG) $(OBJ)
