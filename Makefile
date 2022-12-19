# Makefile for jack_patch

lib.name = jackpatch

class.sources = jackpatch.c

ldlibs = -ljack

datafiles = \
    jackpatch-help.pd \
    jackpatch-meta.pd \
    LICENSE.txt \
    README.txt


# This Makefile is based on the Makefile from pd-lib-builder written by
# Katja Vetter. You can get it from:
# https://github.com/pure-data/pd-lib-builder

PDLIBBUILDER_DIR=pd-lib-builder/
include $(firstword $(wildcard $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder Makefile.pdlibbuilder))
