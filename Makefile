SRC_PATH = src
OBJ_PATH = obj

CC = clang
CFLAGS = -std=c11 -Wall -Wpointer-arith -Wwrite-strings -Wuninitialized -pedantic
LDFLAGS = -static-libgcc -lmingw32 
TARGET = failboy.exe

CFLAGS += -DSDL

LDFLAGS += -static
LDFLAGS += -lSDL2main -lSDL2
LDFLAGS += -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid

DEBUG_CFLAGS = -g3
RELEASE_CFLAGS += -g0 -O3
RELEASE_LDFLAGS += -Wl,--subsystem,windows

SRCS = $(wildcard $(SRC_PATH)/*.c)
OBJS = $(SRCS:$(SRC_PATH)/%.c=$(OBJ_PATH)/%.o)
DEPS = $(OBJS:.o=.d)
RCS = $(wildcard $(SRC_PATH)/*.rc)
RES = $(RCS:$(SRC_PATH)/%.rc=$(OBJ_PATH)/%.res)

vpath %.c $(SRC_PATH)
vpath %.rc $(SRC_PATH)

.PHONY: default clean style

default: $(OBJ_PATH) pre_debug $(TARGET)

release: $(OBJ_PATH) pre_release $(TARGET)
	@echo Stripping $(TARGET)
	@strip -s $(TARGET)
#upx -q --best $(TARGET)

pre_debug:
	$(eval CFLAGS += $(DEBUG_CFLAGS))

pre_release:
	$(eval CFLAGS += $(RELEASE_CFLAGS))
	$(eval LDFLAGS += $(RELEASE_LDFLAGS))

$(OBJ_PATH):
	@mkdir -p $(OBJ_PATH)

$(TARGET): $(OBJS) $(RES)
	@echo Building $@
	@$(CC) $(OBJS) $(RES) $(LDFLAGS) -o $@

$(OBJ_PATH)/%.o: %.c
	@echo Compiling $<
	@$(CC) -c $(CFLAGS) -MMD -o $@ $<

$(OBJ_PATH)/%.res: %.rc
	@echo Building $<
	@windres $< -O coff -o $@

clean:
	@echo Cleaning up.
	@rm -rf $(TARGET) $(OBJ_PATH) *.res

style:
	@echo Styling.
	@astyle -A2 -T -p -U -xe -m1 -k3 -W3 -j -O -n -S -Y -xC120  $(SRCS) $(wildcard $(SRC_PATH)/*.h)

-include $(DEPS)