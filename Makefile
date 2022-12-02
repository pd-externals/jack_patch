# Makefile for mp3cast~

lib.name = jackx

class.sources = \
    jack-connect.c \
    jack-ports.c

shared.sources = \
    libjackx.c

ldlibs = -ljack

datafiles = \
    jack-connect-help.pd \
    jack-ports-help.pd \
    jackx-meta.pd \
    LICENSE.txt \
    README.txt


# This Makefile is based on the Makefile from pd-lib-builder written by
# Katja Vetter. You can get it from:
# https://github.com/pure-data/pd-lib-builder

PDLIBBUILDER_DIR=pd-lib-builder/
include $(firstword $(wildcard $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder Makefile.pdlibbuilder))
