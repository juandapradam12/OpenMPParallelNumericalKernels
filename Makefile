CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -Wpedantic -O2 -Iinclude -fopenmp
LDFLAGS  := -lm -fopenmp

BUILD    := build
BIN      := bin
SRC      := src

.PHONY: all clean dirs run benchmark

all: dirs $(BIN)/advection $(BIN)/walk

dirs:
	@mkdir -p $(BUILD) $(BIN) results figures

$(BUILD)/advection.o: $(SRC)/advection.c include/advection.h include/timing.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/random_walk.o: $(SRC)/random_walk.c include/random_walk.h include/timing.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main_advection.o: $(SRC)/main_advection.c include/advection.h include/timing.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main_walk.o: $(SRC)/main_walk.c include/random_walk.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/advection: $(BUILD)/main_advection.o $(BUILD)/advection.o
	$(CC) $^ -o $@ $(LDFLAGS)

$(BIN)/walk: $(BUILD)/main_walk.o $(BUILD)/random_walk.o
	$(CC) $^ -o $@ $(LDFLAGS)

run: all
	./scripts/run_benchmark.sh

benchmark: run

clean:
	rm -rf $(BUILD) $(BIN)
	rm -f results/*.dat results/timings.csv results/scaling.csv
	rm -f results/large/*.dat
	rm -f figures/advection.png figures/advection.pdf
	rm -f figures/random_walk.png figures/random_walk.pdf
	rm -f figures/scaling.png figures/scaling.pdf
