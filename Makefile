

##### IMPORTANT #####

# If using the runall command,
# the extension will only run on .ecl files, NOT .ncl files.

# This is because the extension uses a modified grammar;
# .ecl files may fail on the standard parser and interpreter.

CC      := clang
DEBUG   := -g3
OPTIM   := -O3
CFLAGS  := -Wall -Wextra -Wpedantic -Wfloat-equal -Wvla -Wuninitialized -std=c99 -Werror
RELEASE := $(CFLAGS) $(OPTIM)
SANI    := $(CFLAGS) -fsanitize=undefined -fsanitize=address $(DEBUG)
VALG    := $(CFLAGS)  $(DEBUG)
NCLS    := $(wildcard *.ncl)
ECLS    := $(wildcard *.ecl)
PRES := $(NCLS:.ncl=.pres)
IRES := $(NCLS:.ncl=.ires)
ERES := $(ECLS:.ecl=.eres)
LIBS    := -lm


parse: nuclei.c general.c nuclei.h
	$(CC) nuclei.c general.c $(RELEASE) -o parse $(LIBS)

parse_s: nuclei.c general.c nuclei.h
	$(CC) nuclei.c general.c $(SANI) -o parse_s $(LIBS)

parse_v: nuclei.c general.c nuclei.h
	$(CC) nuclei.c general.c $(VALG) -o parse_v $(LIBS)

all: parse parse_s parse_v interp interp_s interp_v extension extension_s extension_v

interp: nuclei.c linked.c general.c nuclei.h lisp.h general.h
	$(CC) nuclei.c linked.c general.c $(RELEASE) -DINTERP -o interp $(LIBS)

interp_s: nuclei.c linked.c general.c nuclei.h lisp.h general.h
	$(CC) nuclei.c linked.c general.c $(SANI) -DINTERP -o interp_s $(LIBS)

interp_v: nuclei.c linked.c general.c nuclei.h lisp.h general.h
	$(CC) nuclei.c linked.c general.c $(VALG) -DINTERP -o interp_v $(LIBS)

extension: extension.c linked.c general.c extension.h lisp.h general.h
	$(CC) extension.c linked.c general.c $(RELEASE) -DINTERP -o extension $(LIBS)

extension_s: extension.c linked.c general.c extension.h lisp.h general.h
	$(CC) extension.c linked.c general.c $(SANI) -DINTERP -o extension_s $(LIBS)

extension_v: extension.c linked.c general.c extension.h lisp.h general.h
	$(CC) extension.c linked.c general.c $(VALG) -DINTERP -o extension_v $(LIBS)


# For all .ncl files, run them and output result to a .pres (parse result)
# or .ires (interpretted result).
# For all .ecl files, run them and output result to a .eres (extension result) file.
runall : ./parse_s ./interp_s ./extension_s $(PRES) $(IRES) $(ERES)

%.pres:
	-./parse_s  $*.ncl > $*.pres 2>&1
%.ires:
	-./interp_s $*.ncl > $*.ires 2>&1
%.eres:
	-./extension_s $*.ecl > $*.eres 2>&1

clean:
	rm -f parse parse_s parse_v interp interp_s interp_v extension extension_s extension_v $(PRES) $(IRES) $(ERES)
