CC=gcc
IDIR=.
OBJ_DIR=obj_dir

LIBS=-lncurses -lmenu -lelf
_DEPS = elfibia.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = elfibia.o elfheader.o elfsections.o visual.o
OBJ = $(patsubst %,$(OBJ_DIR)/%,$(_OBJ))

$(OBJ_DIR)/%.o: %.c $(DEPS)
	mkdir --parents ${dir $@}
	$(CC) -c -o $@ $< $(CFLAGS)

elfibia: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm --recursive --force $(OBJ_DIR) elfibia
