#
# Component Makefile
#
# This Makefile can be left empty. By default, it will take the sources in the
# src/ directory, compile them and link them into lib(subdirectory_name).a
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

#CFLAGS :=
#COMPONENT_DEPENDS :=
COMPONENT_SRCDIRS := . audio display input system
COMPONENT_ADD_INCLUDEDIRS := $(COMPONENT_SRCDIRS) ..
