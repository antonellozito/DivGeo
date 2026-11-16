#  VERSION : 02.11.2000 22:29

PROG = dg.exe

# Test whether necessary environment variables are defined; if not, exit
ifndef HOST_NAME
$(error HOST_NAME not defined)
endif
ifndef COMPILER
$(error COMPILER not defined)
endif

MAKETAGS ?= ctags -e -f

MAKES = Makefile 
# Include global SOLPS compiler settings
ifndef SOLPS_CPP
  ifeq ($(shell [ -e ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER} ] && echo yes || echo no ),yes)
    include ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER}
    MAKES += ${SOLPSTOP}/SETUP/setup.csh.${HOST_NAME}.${COMPILER} ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER}
  else
    $(warning ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER} not found. Assuming stand-alone compilation.)
  endif
  ifeq ($(shell [ -e ${SOLPSTOP}/SETUP/config.common.${COMPILER} ] && echo yes || echo no ),yes)
    include ${SOLPSTOP}/SETUP/config.common.${COMPILER}
    MAKES += ${SOLPSTOP}/SETUP/config.common.${COMPILER}
  endif
else
  MAKES += ${SOLPSTOP}/Makefile ${SOLPSTOP}/SETUP/setup.csh.${HOST_NAME}.${COMPILER} ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER}
  ifeq ($(shell [ -e ${SOLPSTOP}/SETUP/config.common.${COMPILER} ] && echo yes || echo no ),yes)
    MAKES += ${SOLPSTOP}/SETUP/config.common.${COMPILER}
  endif
endif
ifeq ($(shell [ -e ${SOLPSTOP}/SETUP/setup.csh.${HOST_NAME}.${COMPILER}.local ] && echo yes || echo no ),yes)
  MAKES += ${SOLPSTOP}/SETUP/setup.csh.${HOST_NAME}.${COMPILER}.local
endif
ifeq ($(shell [ -e ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER}.local ] && echo yes || echo no ),yes)
  include ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER}.local
  MAKES += ${SOLPSTOP}/SETUP/config.${HOST_NAME}.${COMPILER}.local
endif

ifdef SOLPS_DEBUG
EXT_DEBUG = .debug
endif

OBJDIR = ${PWD}/builds/$(HOST_NAME).$(COMPILER)$(EXT_DEBUG)

SHELL  = /bin/sh
DG     = $(OBJDIR)/${PROG}
VPATH  = src

SRCDIR = ${PWD}

ifdef USE_MOTIF
$(shell awk 'FNR==1{if(!/^OBJS *=/){e=1;exit}} END{exit e}' \
        ${OBJDIR}/LISTOBJ 2>/dev/null || rm -f ${OBJDIR}/LISTOBJ)
include ${OBJDIR}/LISTOBJ
endif

ifeq ($(shell [ -e config/config.${HOST_NAME}.${COMPILER} ] && echo yes || echo no ),yes)
include config/config.${HOST_NAME}.${COMPILER}
MAKES+= config/config.${HOST_NAME}.${COMPILER}
else
$(error config/config.${HOST_NAME}.${COMPILER} not found.)
endif

ifeq ($(shell [ -e config/config.common.${COMPILER} ] && echo yes || echo no ),yes)
include config/config.common.${COMPILER}
MAKES+= config/config.common.${COMPILER}
endif

ifeq ($(shell [ -e config/config.${HOST_NAME}.${COMPILER}.local ] && echo yes || echo no ),yes)
include config/config.${HOST_NAME}.${COMPILER}.local
MAKES+= config/config.${HOST_NAME}.${COMPILER}.local
endif

ifdef USE_MOTIF
# DivGeo with the Motif interface
ifneq ($(shell printf '\043include <Xm/Xm.h>\n' | $(CC) $(INCLUDES) -x c -E - >/dev/null 2>&1 && echo yes),yes)
$(error DivGeo with USE_MOTIF needs Motif: Xm/Xm.h not found by $(CC) $(INCLUDES))
endif
GUISTAMP = ${OBJDIR}/.gui_motif

DEST = $(OBJS:%.o=$(OBJDIR)/%.o)

$(OBJDIR)/%.o : %.c
	 $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(DG): $(DEST) $(MAKES) | ${GUISTAMP}
	 $(CC) $(CFLAGS) $(INCLUDES) -o $@ $(DEST) $(LIBS)

all: VERSION listobj depend $(DG)

update: clean VERSION listobj depend all

clean:
	/bin/rm -rf ${OBJDIR}/*.o $(DG) ${OBJDIR}/*.bak src/git_version_DG.h ${OBJDIR}/dependencies* ${OBJDIR}/LISTOBJ ${OBJDIR}/dg.dgc ${OBJDIR}/dg.dgh

depend: ${OBJS:.o=.c}
	@-cd ${OBJDIR} ; ln -sf ${SRCDIR}/src/dg.dgh ${SRCDIR}/dg.dgc .
ifndef NO_MOTIF
	@$(CC) ${INCLUDES} -M $^ | sed '/^[^ ]*.o: / s|^|${OBJDIR}/|' | \
	sed -e 's,^${OBJDIR}/,\$${OBJDIR}/,' | \
	sed 's,: ${SOLPSTOP},: $${SOLPSTOP},' > ${OBJDIR}/dependencies
endif

${OBJDIR}/dependencies:
	-mkdir -p ${OBJDIR}
	-cd ${OBJDIR} ; ln -sf ${SRCDIR}/src/dg.dgh ${SRCDIR}/dg.dgc .
	printf '# Dummy dependencies file for DivGeo\n' > ${OBJDIR}/dependencies
	${MAKE} VERSION
	${MAKE} tags
	${MAKE} listobj
	${MAKE} depend
else
# DivGeo with the Python interface: the core, the resources and src/py as
# a shared library for lib/python/divgeo, without Motif and X11
GUISTAMP = ${OBJDIR}/.gui_python
PYDIR    = ${OBJDIR}/python
ifeq ($(shell uname -s),Darwin)
PYLIB    = ${PYDIR}/libdivgeo.dylib
PYLIBOPT = -dynamiclib
else
PYLIB    = ${PYDIR}/libdivgeo.so
PYLIBOPT = -shared
endif
PYSRC    = $(filter-out src/vacreate.c src/res2fbr.c \
             $(filter-out src/x_~fbr.c,$(wildcard src/x_*.c)),$(wildcard src/*.c)) \
           $(wildcard src/py/*.c)
PYDEST   = $(addprefix ${PYDIR}/,$(notdir $(PYSRC:.c=.o)))
PYINC    = -Isrc/py -Isrc
VPATH   += src/py

${PYLIB}: $(PYDEST) $(MAKES) | ${GUISTAMP}
	 $(CC) $(CFLAGS) $(PYLIBOPT) -o $@ $(PYDEST) -lpthread -lm

${PYDIR}/%.o : %.c | ${PYDIR}
	 $(CC) $(CFLAGS) -fPIC $(PYINC) -c $< -o $@

${PYDIR}:
	mkdir -p $@

all: VERSION depend ${PYLIB}

update: clean VERSION depend all

clean:
	/bin/rm -rf ${PYDIR} src/git_version_DG.h

depend: $(PYSRC)
	@-mkdir -p ${PYDIR}
	@$(CC) $(PYINC) -M $^ | sed '/^[^ ]*.o: / s|^|${PYDIR}/|' | \
	sed -e 's,^${PYDIR}/,\$${PYDIR}/,' | \
	sed 's,: ${SOLPSTOP},: $${SOLPSTOP},' > ${PYDIR}/dependencies

${PYDIR}/dependencies:
	-mkdir -p ${PYDIR}
	printf '# Dummy dependencies file for DivGeo\n' > ${PYDIR}/dependencies
	${MAKE} VERSION
	${MAKE} tags
	${MAKE} depend
endif

# The stamp tells scripts/dg which DivGeo was built last
${GUISTAMP}:
	-mkdir -p ${OBJDIR}
	rm -f ${OBJDIR}/.gui_python ${OBJDIR}/.gui_motif
	touch $@

.PHONY: VERSION clean listobj update depend all neat tags force

neat:
	/bin/rm -rf ${OBJDIR}/*.o ${OBJDIR}/*.bak

tags:
	rm -f TAGS ; ${MAKETAGS} TAGS src/*.c src/*.h dg.dgc || touch TAGS

listobj:
	@P=${OBJDIR}; mkdir -p $${P}; cd src ; rm -f $${P}/LISTOBJ; touch $${P}/LISTOBJ; \
	echo "OBJS =" *.c | sed -e 's/ [^ /]*\// /g' -e 's/\.c/.o/g' -e 's/res2fbr\.o//g' > $${P}/LISTOBJ

VERSION: src/git_version_DG.h

src/git_version_DG.h: force
	@echo "#define GIT_VERSION_DG \"`git describe --tags --dirty --always`\"" > ${OBJDIR}/git_version_new.h
	@if cmp -s ${OBJDIR}/git_version_new.h src/git_version_DG.h; then rm ${OBJDIR}/git_version_new.h; else mv ${OBJDIR}/git_version_new.h src/git_version_DG.h; fi

${OBJDIR}/LISTOBJ: listobj

ifdef USE_MOTIF
$(shell [ -s ${OBJDIR}/dependencies ] || rm -f ${OBJDIR}/dependencies)
include ${OBJDIR}/dependencies
else
$(shell [ -s ${PYDIR}/dependencies ] || rm -f ${PYDIR}/dependencies)
include ${PYDIR}/dependencies
endif


