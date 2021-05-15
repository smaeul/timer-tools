#
# Copyright © 2017-2019 Samuel Holland <samuel@sholland.org>
# SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
#

SRC		 = .
OBJ		 = build
TGT		 = $(OBJ)/target

CROSS_COMPILE	?=
AR		 = $(CROSS_COMPILE)gcc-ar
CC		 = $(CROSS_COMPILE)gcc

HOSTAR		 = ar
HOSTCC		 = cc

COMMON_CFLAGS	 = -Os -pipe -std=c11 \
		   -fdata-sections \
		   -ffunction-sections \
		   -fno-common \
		   -fvar-tracking-assignments \
		   -g$(if $(DEBUG),gdb,0) \
		   -pthread \
		   -Wall -Wextra -Wformat=2 -Wpedantic -Wshadow \
		   -Werror=implicit-function-declaration \
		   -Werror=implicit-int \
		   -Werror=pointer-arith \
		   -Werror=pointer-sign \
		   -Werror=strict-prototypes \
		   -Werror=undef \
		   -Werror=vla \
		   -Wno-missing-field-initializers
COMMON_CPPFLAGS	 = -D_GNU_SOURCE \
		   -D_XOPEN_SOURCE=700 \
		   -I$(SRC)/include

AFLAGS		 = -Wa,--fatal-warnings
CFLAGS		 = $(COMMON_CFLAGS)
CPPFLAGS	 = $(COMMON_CPPFLAGS) \
		   -include $(SRC)/include/compiler.h
LDFLAGS		 = -Wl,-O1 \
		   -Wl,--fatal-warnings \
		   -Wl,--gc-sections
LDLIBS		 =

HOSTCFLAGS	 = $(COMMON_CFLAGS)
HOSTCPPFLAGS	 = $(COMMON_CPPFLAGS)
HOSTLDFLAGS	 =
HOSTLDLIBS	 =

###############################################################################

.DEFAULT_GOAL	:= all
MAKEFLAGS	+= -Rr

include $(SRC)/scripts/Makefile.kbuild

$(call descend,src)

###############################################################################

M := @$(if $(filter-out 0,$(V)),:,exec printf '  %-7s %s\n')
Q :=  $(if $(filter-out 0,$(V)),,@)exec

all: $(bin-all)

check: $(test-all:%=%.test)

clean:
	$(Q) rm -fr $(TGT)

clobber:
	$(Q) rm -fr $(OBJ)

distclean:
	$(Q) rm -fr $(OBJ)

format: $(wildcard $(SRC)/include/*.h $(SRC)/src/*.c)
	$(Q) clang-format -i $^

tools: $(tools-all)

%/.:
	$(Q) mkdir -p $*

%.d:;

$(OBJ)/%.test: $(SRC)/scripts/test.sh $(OBJ)/%
	$(M) TEST $@
	$(Q) $^ $@

$(OBJ)/%: $(OBJ)/%.o
	$(M) HOSTLD $@
	$(Q) $(HOSTCC) $(HOSTCFLAGS) $(HOSTLDFLAGS) -o $@ $^ $(HOSTLDLIBS)

$(OBJ)/%.o: $(SRC)/%.c
	$(M) HOSTCC $@
	$(Q) $(HOSTCC) $(HOSTCPPFLAGS) $(HOSTCFLAGS) -MMD -c -o $@ $<

$(TGT)/%: $(TGT)/%.o
	$(M) LD $@
	$(Q) $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(TGT)/%.o: $(SRC)/%.c
	$(M) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(AFLAGS) -MMD -c -o $@ $<

$(TGT)/%.o: $(SRC)/%.S
	$(M) AS $@
	$(Q) $(CC) $(CPPFLAGS) $(AFLAGS) -MMD -c -o $@ $<

$(SRC)/Makefile:;
$(SRC)/%.h:;

.PHONY: all check clean distclean format tools
.SECONDARY:
.SUFFIXES:
