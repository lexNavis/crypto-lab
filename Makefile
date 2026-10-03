CC       = gcc
CFLAGS = -O2 -g -march=native -Wall -Wextra
LDFLAGS  =
LDLIBS   = -lgmp -lpthread -lcjson

BUILD    = build
OBJDIR   = $(BUILD)/obj
BIN      = $(BUILD)/main

SRCS     = main.c \
           common/hex.c common/entropy.c common/config.c \
           kuznyechik/kuznyechik.c \
           rsa/prime.c rsa/rsa.c \
           streebog/streebog.c \
           tests/test_kuz.c tests/args.c tests/test_rsa.c tests/test_streebog.c tests/test_bignum.c \
           bignum/bignum.c

OBJS     = $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(BIN)

$(BIN): $(OBJS) | $(BUILD)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: all clean