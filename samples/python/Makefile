PY_SRCS    = $(filter-out runner.py, $(wildcard *.py))
ELFS       = $(PY_SRCS:.py=.elf)
DOCKER_SRCS = $(PY_SRCS)

.PHONY: all docker-all clean
.SUFFIXES:

all: $(ELFS)

docker-all:
	@for src in $(DOCKER_SRCS); do \
		name=$$(basename $$src .py); \
		echo "Building Docker image for $$name..."; \
		docker build --build-arg SOURCE=$$src -t iai_python_$$name .; \
	done

%.elf: %.py
	bash build.sh $<

clean:
	rm -f $(ELFS)
