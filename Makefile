BIN_NAME := utest

BIN      := /tmp/aex2/bin/$(BIN_NAME)/
BIN_OBJ  := $(BIN)$(BIN_NAME)
DEP_DEST := $(BIN)dep/
OBJ_DEST := $(BIN)obj/

CFILES   := $(shell find . -type f -name '*.c')
HFILES   := $(shell find . -type f -name '*.h')
OBJS     := $(patsubst %.o, $(OBJ_DEST)%.o, $(CFILES:.c=.c.o))

CFLAGS   := -O1 -pipe -flto -std=c11 -g
INCLUDES := -I. -Iinclude/

MKDIR := mkdir -p

format:
	@$(MKDIR) $(BIN)
	clang-format -style=file -i ${CFILES} ${HFILES}

all: $(OBJS)
	@$(MKDIR) $(BIN)
	@$(CC) $(CFLAGS) -o $(BIN_OBJ) $(OBJS)
	
	@printf '\033[0;92m%-10s\033[0m: Done building\033[0K\n' $(BIN_NAME)

copy:
	@cp -u $(BIN_OBJ) "$(COPY_DIR)"

clean:
	rm -rf $(BIN)

include $(shell find $(DEP_DEST) -type f -name *.d)
$(OBJ_DEST)%.c.o : %.c
	@$(MKDIR) ${@D}
	@$(MKDIR) $(dir $(DEP_DEST)$*)

	@printf '\033[0;92m$(BIN_NAME)\033[0m: Building \033[0;92m$(<)\033[0m\033[0K\r'
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MT $@ -MF $(DEP_DEST)$*.c.d