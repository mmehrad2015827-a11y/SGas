# Makefile for SGas
CC      ?= cc
CXX     ?= c++
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
CXXFLAGS?= -std=c++17 -Wall -Wextra -O2
INCS    := -Iinclude \
           -Isrc/common -Isrc/lexer -Isrc/ast -Isrc/parser \
           -Isrc/compiler -Isrc/vm -Isrc/runtime -Isrc/stdlib
LDFLAGS := -lm

BUILD   := build

C_SRCS  := src/common/common.c \
           src/common/value.c \
           src/lexer/lexer.c \
           src/ast/ast.c \
           src/parser/parser.c \
           src/compiler/compiler.c \
           src/vm/vm.c \
           src/runtime/runtime.c \
           src/stdlib/stdlib.c \
           src/main.c

C_OBJS  := $(patsubst src/%.c,$(BUILD)/%.o,$(C_SRCS))

BIN     := $(BUILD)/sgas

.PHONY: all clean test run fmt

all: $(BIN)

$(BIN): $(C_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

test: all
	$(CC) $(CFLAGS) $(INCS) tests/lexer_test.c  $(C_OBJS) -o $(BUILD)/lexer_test  $(LDFLAGS)
	$(CC) $(CFLAGS) $(INCS) tests/parser_test.c $(C_OBJS) -o $(BUILD)/parser_test $(LDFLAGS)
	$(CC) $(CFLAGS) $(INCS) tests/vm_test.c     $(C_OBJS) -o $(BUILD)/vm_test     $(LDFLAGS)
	@$(BUILD)/lexer_test
	@$(BUILD)/parser_test
	@$(BUILD)/vm_test

run: all
	@$(BIN) examples/hello.sgas
	@echo "---"
	@$(BIN) examples/fib.sgas

clean:
	rm -rf $(BUILD)

fmt:
	@echo "(no formatter configured for v0.1)"