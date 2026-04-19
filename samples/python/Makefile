PY_SRCS = $(wildcard *.py)
ELFS = $(PY_SRCS:.py=.elf)

.PHONY: all clean

all: $(ELFS)

%.elf: %.py
	bash build.sh $<

clean:
	rm -f $(ELFS)
