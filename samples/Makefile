# Consolidated Makefile for IaiServerless Samples

.PHONY: all c cpp python proc docker-all clean

all: c cpp python

c:
	$(MAKE) -C c

cpp:
	$(MAKE) -C cpp

python:
	$(MAKE) -C python

proc:
	$(MAKE) -C c proc

docker-all:
	$(MAKE) -C c docker-all
	$(MAKE) -C cpp docker-all

clean:
	$(MAKE) -C c clean
	$(MAKE) -C cpp clean
	$(MAKE) -C python clean
