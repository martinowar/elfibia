CC=gcc
IDIR=.
OBJ_DIR=obj_dir
CFLAGS += -Wall -Werror

LIBS=-lncurses -lmenu -lelf
_DEPS = elfibia.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = elfibia.o elfheader.o elfsections.o elfsegments.o draw-ncurses.o
OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o: %.c $(DEPS) | $(OBJ_DIR)
	$(CC) -c -o $@ $< $(CFLAGS)

elfibia: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(OBJ_DIR):
	mkdir --parents $(OBJ_DIR)

.PHONY: clean

clean:
	rm $(OBJ_DIR)/*.o
	rm --recursive $(OBJ_DIR)
	rm --force $(OBJ_DIR) elfibia
