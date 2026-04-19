# Consolidated Makefile for IaiServerless Samples

.PHONY: all c cpp proc docker-all clean

all: c cpp

c:
	$(MAKE) -C c

cpp:
	$(MAKE) -C cpp

proc:
	$(MAKE) -C c proc

docker-all:
	$(MAKE) -C c docker-all
	$(MAKE) -C cpp docker-all

clean:
	$(MAKE) -C c clean
	$(MAKE) -C cpp clean
