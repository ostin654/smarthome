
ifeq ($(wildcard ../../RF24/Makefile.inc), )
	$(error Configuration not found. Run ./configure first)
endif

include ../../RF24/Makefile.inc

# define all programs
PROGRAMS = well

include Makefile.examples
