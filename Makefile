CC       = gcc
CFLAGS   = -O2 -march=native -Wall -Wextra -I./common -I./rsa
LDFLAGS  = -L/usr/local/lib
LDLIBS = -lgmp -lpthread

BUILD    = build

all: $(BUILD)/rsa

$(BUILD)/rsa: rsa/*.c common/*.c main.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: all clean
