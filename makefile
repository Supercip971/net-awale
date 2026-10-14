MKCWD=mkdir -p $(@D)

CC ?= gcc

CFLAGS_WARNS ?= 	\
		-Wextra 	\
		-Wall 		\
		-Wundef 	\
		-Wshadow 	\
		-Wvla

CFLAGS = 			\
		-O2 		\
		-g 		 	\
		-std=gnu2x 	\
		-Isrc/      \
		-Isrc/shared/json/ \
		$(CFLAGS_WARNS)

LDFLAGS= -lm

PROJECT_NAME = test
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
SRC_DIR = src
SRC_SHARED = src/shared
SRC_DIR_CLIENT = src/client
SRC_DIR_SERVER = src/server

CFILES_CLIENT = $(wildcard $(SRC_DIR_CLIENT)/*.c) $(wildcard $(SRC_DIR_CLIENT)/*/*.c) $(wildcard $(SRC_DIR_CLIENT)/*/*/*.c)
DFILES_CLIENT = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.d, $(CFILES_CLIENT))
OFILES_CLIENT = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CFILES_CLIENT))

CFILES_SERVER = $(wildcard $(SRC_DIR_SERVER)/*.c) $(wildcard $(SRC_DIR_SERVER)/*/*.c) $(wildcard $(SRC_DIR_SERVER)/*/*/*.c)
DFILES_SERVER = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.d, $(CFILES_SERVER))
OFILES_SERVER = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CFILES_SERVER))

CFILES_SHARED = $(wildcard $(SRC_SHARED)/*.c) $(wildcard $(SRC_SHARED)/*/*.c) $(wildcard $(SRC_SHARED)/*/*/*.c)
DFILES_SHARED = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.d, $(CFILES_SHARED))
OFILES_SHARED = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(CFILES_SHARED))

OUTPUT_SERVER = $(BUILD_DIR)/server
OUTPUT_CLIENT = $(BUILD_DIR)/client

all: $(OUTPUT_CLIENT) $(OUTPUT_SERVER)

$(OUTPUT_CLIENT): $(OFILES_CLIENT) $(OFILES_SHARED)
	@$(MKCWD)
	@echo " LD [ $@ ] $<"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(OUTPUT_SERVER): $(OFILES_SERVER) $(OFILES_SHARED)
	@$(MKCWD)
	@echo " LD [ $@ ] $<"
	@$(CC) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(MKCWD)
	@echo " CC [ $@ ] $<"
	@$(CC) $(CFLAGS) -MMD -MP $< -c -o $@

client: $(OUTPUT_CLIENT)
	@$(OUTPUT_CLIENT) $(ARGS)

server: $(OUTPUT_SERVER)
	@$(OUTPUT_SERVER)

N ?= 3

run: all
	@./run_tmux.sh $(N)

VALGRIND ?= valgrind
VALGRIND_FLAGS ?= --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1
VALGRIND_BUILD_DIR = build-valgrind

valgrind-server:
	@$(MAKE) --no-print-directory SAN=0 BUILD_DIR=$(VALGRIND_BUILD_DIR) $(VALGRIND_BUILD_DIR)/server
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(VALGRIND_BUILD_DIR)/server

valgrind-client:
	@$(MAKE) --no-print-directory SAN=0 BUILD_DIR=$(VALGRIND_BUILD_DIR) $(VALGRIND_BUILD_DIR)/client
	@$(VALGRIND) $(VALGRIND_FLAGS) ./$(VALGRIND_BUILD_DIR)/client $(ARGS)

leaks-server:
	@$(MAKE) --no-print-directory SAN=0 BUILD_DIR=$(VALGRIND_BUILD_DIR) $(VALGRIND_BUILD_DIR)/server
	@leaks --atExit -- ./$(VALGRIND_BUILD_DIR)/server

leaks-client:
	@$(MAKE) --no-print-directory SAN=0 BUILD_DIR=$(VALGRIND_BUILD_DIR) $(VALGRIND_BUILD_DIR)/client
	@leaks --atExit -- ./$(VALGRIND_BUILD_DIR)/client $(ARGS)

clean:
	@rm -rf $(BUILD_DIR)/ $(VALGRIND_BUILD_DIR)/

.PHONY: clean all run client server tmux valgrind-server valgrind-client leaks-server leaks-client

-include $(DFILES_CLIENT) $(DFILES_SERVER) $(DFILES_SHARED)
