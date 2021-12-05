SRC = $(wildcard src/*.c)

OBJ = $(patsubst %.c, %.o, $(SRC))

CFLAGS = -Wall -Wextra -Werror=implicit-function-declaration -Wno-unused-function -g -Iinclude

TARG = dist/compiler

.PHONY: all clean

all: $(TARG)

$(TARG): $(OBJ)
	@mkdir -p dist
	@echo "LD    $@"
	@$(CC) -o $@ $^

%.o: %.c
	@echo "CC    $@"
	@$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm $(TARG) $(OBJ)
