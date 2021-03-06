# Makefile for program source directory in GNU NLS utilities package.
# Copyright (C) 1995, 1996, 1997 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file file be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.
#
# - Modified by Owen Taylor <otaylor@redhat.com> to use GETTEXT_PACKAGE
#   instead of PACKAGE and to look for po2tbl in ./ not in intl/
#
# - Modified by jacob berkman <jacob@ximian.com> to install
#   Makefile.in.in and po2tbl.sed.in for use with glib-gettextize
#
# - Modified by jwz to not suck wet farts from dead pigeons.
#   This is truly one of the worst-written Makefiles I've ever seen.
#   Problems I (tried to) fix:
#
#     - MKINSTALLDIRS was $(top_srcdir)/${INSTALL} -d, but the
#       configure script already put $(top_srcdir) into ${INSTALL} -d,
#       so the result was an absolute path that didn't work at all.
#
#     - GMSGFMT and XGETTEXT set $PATH to some random value before
#       running the program.  That's completely stupid, and anyway,
#       the hardcoded $PATH value they were using (../src) do not
#       apply to xscreensaver.
#
#     - the .po.gmo, .po.cat, .po.pox, .pot, install-data-yes, update-po, and
#	uninstall rules didn't actually print the commands they were
#	executing: they just dumped a bunch of sh code at you, and expected
#	you to figure it out yourself.  This is typical behavior of Makefile
#	rules written by morons.
#
#     - intltool-update prints a lot of useless crap to both stderr and
#	stdout and has no command-line argument meaning "be quiet, and only
#	print errors", so I just redirected the whole thing to /dev/null.
#
#     - Demonstrating a total misunderstanding of build-directories and
#       VPATH, the .po.gmo rule was writing output to $(srcdir).  When
#       the current directory is not $(srcdir), one *never* writes to
#       $(srcdir) -- it is to be considered read-only.
#
#     - Likewise, install-data-yes was looking for .gmo files in both the
#       current directory, and in $(srcdir).  In the case where those are
#       not the same directory, there must be no .gmo files in $(srcdir).
#
#     - The update-po rule was explicitly writing to $(srcdir).  I changed
#       it to just work in the current directory.  As I understand it, it
#       only makes sense to run this rule while in the srcdir, e.g., from
#       distdepend -- so it should be allowed to error out when run in an
#       inappropriate directory.
#
#     - install-data-yes went through great code-duplication-contortions
#       to cope with the file mentioned by MKINSTALLDIRS not actually
#       existing!  Instead of assuming that MKINSTALLDIRS will be set to
#       an actual, correct value, it had a hardcoded check to look for it
#       in $(top_srcdir) as well!  WTF!
#
#     - It turns out that "msgfmt" silently fails to write a .gmo file
#       if the source .po file is empty (or otherwise not yet written.)
#       The install-data-yes rule made no provision for this, and would
#       just error out when the .gmo files were not there.
#
#     - $(DESTDIR) was being used where $(install_prefix) should have been.
#
#     - For xscreensaver, $(prefix) is generally /usr/X11R6/, but some files
#	need to go under /usr/ instead of under /usr/X11R6/ -- for those, we
#	use /usr/share instead of /usr/local.  So I changed $(datadir) and
#	$(gnulocaledir) appropriately.
#
#     - WTF is $(gettextsrcdir) doing in here?
#
#     - added  "*.gmo *.pot" to "distclean".
#
#     - I added a rule to po/POTFILES.in so that I wouldn't have to maintain
#       that list of files in two places.
#   
#     - I removed the rule to auto-regenerate Makefile from Makefile.in.in
#       because it made my top-level "make distdepend" rule fail: things
#       were executing in the wrong order.
#
#     - the .po.cat rule has a hardcoded "../intl/po2msg.sed" in it.
#       Guess what, I don't have any such directory.  And that should be
#       refering to $(srcdir) anyway.  So I just commented (my half-fixed
#       version of) this rule out.
#

GETTEXT_PACKAGE = xscreensaver
PACKAGE = @PACKAGE@
VERSION = @VERSION@

SHELL = /bin/sh


srcdir = .
top_srcdir = ..
top_builddir = ..


prefix = /usr/local
exec_prefix = ${prefix}
#datadir = $(prefix)/share      -- not for xscreensaver
datadir = /usr/share
datarootdir = ${prefix}/share
localedir = $(datadir)/locale
#localedir = $(prefix)/share/locale
#gnulocaledir = $(prefix)/share/locale  -- not for xscreensaver
gnulocaledir = $(localedir)
#gettextsrcdir = $(prefix)/share/glib-2.0/gettext/po
subdir = po
DESTDIR = $(install_prefix)

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

# #### NO.  This gets me "..//usr/bin/install: No such file or directory".
# The ${INSTALL} -d variable must have the absolute (or unqualified)
# path of the install program.  --jwz.
# MKINSTALLDIRS = $(top_builddir)/${INSTALL} -d

MKINSTALLDIRS = ${INSTALL} -d

CC = gcc -pedantic -Wall -Wstrict-prototypes -Wnested-externs -Wmissing-prototypes -Wno-overlength-strings -Wdeclaration-after-statement -std=c89 -U__STRICT_ANSI__
#GENCAT = @GENCAT@
GMSGFMT = /usr/bin/msgfmt
MSGFMT = /usr/bin/msgfmt
XGETTEXT = /usr/bin/xgettext
INTLTOOL_UPDATE = $(top_builddir)/intltool-update
INTLTOOL_EXTRACT = $(top_builddir)/intltool-extract
MSGMERGE = $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --dist
GENPOT   = $(INTLTOOL_UPDATE) --gettext-package $(GETTEXT_PACKAGE) --pot

DEFS = -DHAVE_CONFIG_H
CFLAGS = -g -O2
CPPFLAGS = 

INCLUDES = -I.. -I$(top_srcdir)/intl

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

SOURCES = 
POFILES =  ca.po da.po de.po es.po et.po fi.po fr.po hu.po it.po ja.po ko.po nb.po nl.po pl.po pt.po pt_BR.po ru.po sk.po sv.po vi.po wa.po zh_CN.po zh_TW.po
GMOFILES =  ca.gmo da.gmo de.gmo es.gmo et.gmo fi.gmo fr.gmo hu.gmo it.gmo ja.gmo ko.gmo nb.gmo nl.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sv.gmo vi.gmo wa.gmo zh_CN.gmo zh_TW.gmo
DISTFILES_1 = ChangeLog Makefile.in.in POTFILES.in update.sh
DISTFILES = $(DISTFILES_1) $(GETTEXT_PACKAGE).pot \
	    $(POFILES) $(GMOFILES) $(SOURCES)

TARFILES = $(DISTFILES_1) $(POFILES) $(SOURCES)

POTFILES = \
	../driver/demo-Gtk-conf.c \
	../driver/demo-Gtk-support.c \
	../driver/demo-Gtk-widgets.c \
	../driver/demo-Gtk.c \
	../driver/screensaver-properties.desktop.in \
	../driver/xscreensaver-demo.glade2 \
	../hacks/config/abstractile.xml \
	../hacks/config/anemone.xml \
	../hacks/config/anemotaxis.xml \
	../hacks/config/ant.xml \
	../hacks/config/antinspect.xml \
	../hacks/config/antmaze.xml \
	../hacks/config/antspotlight.xml \
	../hacks/config/apollonian.xml \
	../hacks/config/apple2.xml \
	../hacks/config/atlantis.xml \
	../hacks/config/attraction.xml \
	../hacks/config/atunnel.xml \
	../hacks/config/barcode.xml \
	../hacks/config/blaster.xml \
	../hacks/config/blinkbox.xml \
	../hacks/config/blitspin.xml \
	../hacks/config/blocktube.xml \
	../hacks/config/boing.xml \
	../hacks/config/bouboule.xml \
	../hacks/config/bouncingcow.xml \
	../hacks/config/boxed.xml \
	../hacks/config/boxfit.xml \
	../hacks/config/braid.xml \
	../hacks/config/bsod.xml \
	../hacks/config/bubble3d.xml \
	../hacks/config/bubbles.xml \
	../hacks/config/bumps.xml \
	../hacks/config/cage.xml \
	../hacks/config/carousel.xml \
	../hacks/config/ccurve.xml \
	../hacks/config/celtic.xml \
	../hacks/config/circuit.xml \
	../hacks/config/cloudlife.xml \
	../hacks/config/compass.xml \
	../hacks/config/coral.xml \
	../hacks/config/crackberg.xml \
	../hacks/config/critical.xml \
	../hacks/config/crystal.xml \
	../hacks/config/cube21.xml \
	../hacks/config/cubenetic.xml \
	../hacks/config/cubestorm.xml \
	../hacks/config/cubicgrid.xml \
	../hacks/config/cwaves.xml \
	../hacks/config/cynosure.xml \
	../hacks/config/dangerball.xml \
	../hacks/config/decayscreen.xml \
	../hacks/config/deco.xml \
	../hacks/config/deluxe.xml \
	../hacks/config/demon.xml \
	../hacks/config/discrete.xml \
	../hacks/config/distort.xml \
	../hacks/config/dnalogo.xml \
	../hacks/config/drift.xml \
	../hacks/config/endgame.xml \
	../hacks/config/engine.xml \
	../hacks/config/epicycle.xml \
	../hacks/config/eruption.xml \
	../hacks/config/euler2d.xml \
	../hacks/config/extrusion.xml \
	../hacks/config/fadeplot.xml \
	../hacks/config/fiberlamp.xml \
	../hacks/config/fireworkx.xml \
	../hacks/config/flag.xml \
	../hacks/config/flame.xml \
	../hacks/config/flipflop.xml \
	../hacks/config/flipscreen3d.xml \
	../hacks/config/fliptext.xml \
	../hacks/config/flow.xml \
	../hacks/config/fluidballs.xml \
	../hacks/config/flurry.xml \
	../hacks/config/flyingtoasters.xml \
	../hacks/config/fontglide.xml \
	../hacks/config/forest.xml \
	../hacks/config/fuzzyflakes.xml \
	../hacks/config/galaxy.xml \
	../hacks/config/gears.xml \
	../hacks/config/gflux.xml \
	../hacks/config/glblur.xml \
	../hacks/config/glcells.xml \
	../hacks/config/gleidescope.xml \
	../hacks/config/glforestfire.xml \
	../hacks/config/glhanoi.xml \
	../hacks/config/glknots.xml \
	../hacks/config/glmatrix.xml \
	../hacks/config/glplanet.xml \
	../hacks/config/glschool.xml \
	../hacks/config/glslideshow.xml \
	../hacks/config/glsnake.xml \
	../hacks/config/gltext.xml \
	../hacks/config/goop.xml \
	../hacks/config/grav.xml \
	../hacks/config/greynetic.xml \
	../hacks/config/halftone.xml \
	../hacks/config/halo.xml \
	../hacks/config/helix.xml \
	../hacks/config/hopalong.xml \
	../hacks/config/hyperball.xml \
	../hacks/config/hypercube.xml \
	../hacks/config/hypertorus.xml \
	../hacks/config/hypnowheel.xml \
	../hacks/config/ifs.xml \
	../hacks/config/imsmap.xml \
	../hacks/config/interaggregate.xml \
	../hacks/config/interference.xml \
	../hacks/config/intermomentary.xml \
	../hacks/config/jigglypuff.xml \
	../hacks/config/jigsaw.xml \
	../hacks/config/juggle.xml \
	../hacks/config/juggler3d.xml \
	../hacks/config/julia.xml \
	../hacks/config/kaleidescope.xml \
	../hacks/config/klein.xml \
	../hacks/config/kumppa.xml \
	../hacks/config/lament.xml \
	../hacks/config/laser.xml \
	../hacks/config/lavalite.xml \
	../hacks/config/lcdscrub.xml \
	../hacks/config/lightning.xml \
	../hacks/config/lisa.xml \
	../hacks/config/lissie.xml \
	../hacks/config/lmorph.xml \
	../hacks/config/lockward.xml \
	../hacks/config/loop.xml \
	../hacks/config/m6502.xml \
	../hacks/config/maze.xml \
	../hacks/config/memscroller.xml \
	../hacks/config/menger.xml \
	../hacks/config/metaballs.xml \
	../hacks/config/mirrorblob.xml \
	../hacks/config/mismunch.xml \
	../hacks/config/moebius.xml \
	../hacks/config/moebiusgears.xml \
	../hacks/config/moire.xml \
	../hacks/config/moire2.xml \
	../hacks/config/molecule.xml \
	../hacks/config/morph3d.xml \
	../hacks/config/mountain.xml \
	../hacks/config/munch.xml \
	../hacks/config/nerverot.xml \
	../hacks/config/noof.xml \
	../hacks/config/noseguy.xml \
	../hacks/config/pacman.xml \
	../hacks/config/pedal.xml \
	../hacks/config/penetrate.xml \
	../hacks/config/penrose.xml \
	../hacks/config/petri.xml \
	../hacks/config/phosphor.xml \
	../hacks/config/photopile.xml \
	../hacks/config/piecewise.xml \
	../hacks/config/pinion.xml \
	../hacks/config/pipes.xml \
	../hacks/config/polyhedra.xml \
	../hacks/config/polyominoes.xml \
	../hacks/config/polytopes.xml \
	../hacks/config/pong.xml \
	../hacks/config/popsquares.xml \
	../hacks/config/providence.xml \
	../hacks/config/pulsar.xml \
	../hacks/config/pyro.xml \
	../hacks/config/qix.xml \
	../hacks/config/queens.xml \
	../hacks/config/rd-bomb.xml \
	../hacks/config/rdbomb.xml \
	../hacks/config/ripples.xml \
	../hacks/config/rocks.xml \
	../hacks/config/rorschach.xml \
	../hacks/config/rotor.xml \
	../hacks/config/rotzoomer.xml \
	../hacks/config/rubik.xml \
	../hacks/config/rubikblocks.xml \
	../hacks/config/sballs.xml \
	../hacks/config/shadebobs.xml \
	../hacks/config/sierpinski.xml \
	../hacks/config/sierpinski3d.xml \
	../hacks/config/skytentacles.xml \
	../hacks/config/slidescreen.xml \
	../hacks/config/slip.xml \
	../hacks/config/sonar.xml \
	../hacks/config/speedmine.xml \
	../hacks/config/sphere.xml \
	../hacks/config/spheremonics.xml \
	../hacks/config/spiral.xml \
	../hacks/config/spotlight.xml \
	../hacks/config/sproingies.xml \
	../hacks/config/squiral.xml \
	../hacks/config/stairs.xml \
	../hacks/config/starfish.xml \
	../hacks/config/starwars.xml \
	../hacks/config/stonerview.xml \
	../hacks/config/strange.xml \
	../hacks/config/substrate.xml \
	../hacks/config/superquadrics.xml \
	../hacks/config/surfaces.xml \
	../hacks/config/swirl.xml \
	../hacks/config/t3d.xml \
	../hacks/config/tangram.xml \
	../hacks/config/thornbird.xml \
	../hacks/config/timetunnel.xml \
	../hacks/config/topblock.xml \
	../hacks/config/triangle.xml \
	../hacks/config/truchet.xml \
	../hacks/config/twang.xml \
	../hacks/config/vermiculate.xml \
	../hacks/config/vidwhacker.xml \
	../hacks/config/vines.xml \
	../hacks/config/voronoi.xml \
	../hacks/config/wander.xml \
	../hacks/config/webcollage.xml \
	../hacks/config/whirlwindwarp.xml \
	../hacks/config/whirlygig.xml \
	../hacks/config/worm.xml \
	../hacks/config/wormhole.xml \
	../hacks/config/xanalogtv.xml \
	../hacks/config/xflame.xml \
	../hacks/config/xjack.xml \
	../hacks/config/xlyap.xml \
	../hacks/config/xmatrix.xml \
	../hacks/config/xrayswarm.xml \
	../hacks/config/xspirograph.xml \
	../hacks/config/zoom.xml 

CATALOGS =  ca.gmo da.gmo de.gmo es.gmo et.gmo fi.gmo fr.gmo hu.gmo it.gmo ja.gmo ko.gmo nb.gmo nl.gmo pl.gmo pt.gmo pt_BR.gmo ru.gmo sk.gmo sv.gmo vi.gmo wa.gmo zh_CN.gmo zh_TW.gmo
CATOBJEXT = .gmo
INSTOBJEXT = .mo

.SUFFIXES:
.SUFFIXES: .c .o .po .pox .gmo .mo .msg .cat

.c.o:
	$(COMPILE) $<

.po.pox:
	$(MAKE) $(GETTEXT_PACKAGE).pot
	INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) ; \
	export INTLTOOL_EXTRACT ; \
	echo $(MSGMERGE) $< $(srcdir)/$(GETTEXT_PACKAGE).pot -o $*.pox ; \
	     $(MSGMERGE) $< $(srcdir)/$(GETTEXT_PACKAGE).pot -o $*.pox

.po.mo:
	$(MSGFMT) -o $@ $<

# jwz: the "-" means "ignore any errors here, because I don't give a shit."
.po.gmo:
	@-file=`echo $* | sed 's,.*/,,'`.gmo ; \
	 rm -f "$$file" ; \
	 echo $(GMSGFMT) -o "$$file" $< ; \
	      $(GMSGFMT) -o "$$file" $<

#.po.cat:
#	@echo "sed -f ../intl/po2msg.sed < $< > $*.msg" ; \
#	       sed -f ../intl/po2msg.sed < $< > $*.msg && \
#	 rm -f $@ && \
#	 echo $(GENCAT) "$@" "$*.msg" ; \
#	      $(GENCAT) "$@" "$*.msg"


all: all-yes
default: all

all-yes: $(CATALOGS)
all-no:

$(GETTEXT_PACKAGE).pot: $(POTFILES)
	@INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) ; \
	export INTLTOOL_EXTRACT ; \
	echo top_srcdir=$(top_srcdir) $(GENPOT) ; \
	     top_srcdir=$(top_srcdir) $(GENPOT)

install-strip: install

install: install-exec install-data
install-exec:
install-program:
install-man:
install-strip:
tags:
install-data: install-data-yes
install-data-no: all
install-data-yes: all
	@set -e; \
	catalogs='$(CATALOGS)'; \
	losers=""; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  case "$$cat" in \
	    *.gmo) destdir=$(gnulocaledir);; \
	    *)     destdir=$(localedir);; \
	  esac; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  dir=$(install_prefix)$$destdir/$$lang/LC_MESSAGES; \
	  dest=$$dir/$(GETTEXT_PACKAGE)$(INSTOBJEXT); \
	  if test -r $$cat -o -r $$cat.m; then \
	    echo $(MKINSTALLDIRS) $$dir; \
	         $(MKINSTALLDIRS) $$dir; \
	  else \
	    losers="$$losers $$lang"; \
	  fi ; \
	  if test -r $$cat; then \
	    echo $(INSTALL_DATA) $$cat $$dest; \
		 $(INSTALL_DATA) $$cat $$dest; \
	  fi; \
	  if test -r $$cat.m; then \
	    echo $(INSTALL_DATA) $$cat.m $$dest.m; \
		 $(INSTALL_DATA) $$cat.m $$dest.m; \
	  fi; \
	done; \
	if [ "$$losers" != '' ]; then \
	  echo "Warning: these catalogs were not installed:" >&2 ; \
	  echo "	$$losers" >&2; \
	fi

# Define this as empty until I found a useful application.
installcheck:

uninstall:
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  suffix="$$lang/LC_MESSAGES/$(GETTEXT_PACKAGE)$(INSTOBJEXT)"; \
	  for f in $(install_prefix)$(localedir)/$$suffix \
		   $(install_prefix)$(localedir)/$$suffix.m \
		   $(install_prefix)$(gnulocaledir)/$$suffix \
		   $(install_prefix)$(gnulocaledir)/$$suffix.m; do \
	    if [ -f $$f ]; then \
	      echo rm -f $$f; \
	           rm -f $$f; \
	    fi; \
	  done; \
	done

uninstall-program:
uninstall-man:

check: all

dvi info tags TAGS ID:

mostlyclean:
	rm -f core core.* *.pox $(GETTEXT_PACKAGE).po *.old.po cat-id-tbl.tmp
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo *.msg *.cat *.cat.m *.gmo *.pot
	rm -f stamp-it

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f $(GMOFILES)

depend:
distdepend: generate_potfiles_in update-po $(DISTFILES)


# jwz: Generates po/POTFILES.in by examining the source tree:
# that way we don't have to keep this list up to date as files are added.
#
generate_potfiles_in:
	@tmp=po.$$$$ ;				\
	 file=POTFILES.in ;			\
	 echo "sleeping 3 seconds..." ;\
	 sleep 3; \
	 ( cd $(top_srcdir);			\
	   echo "# Auto-generated: `LANG=C date`" ;	\
	   ( grep -l '_("' driver/*.[ch] ;	\
	     ls driver/*.glade2			\
		driver/*.desktop.in		\
		hacks/config/*.xml ) | sort	\
	  ) > $$tmp &&				\
	 mv $$tmp $$file &&			\
	 echo "created `pwd`/$$file"

echo_tarfiles:
	@echo $(TARFILES)

distdir = ../$(GETTEXT_PACKAGE)-$(VERSION)/$(subdir)
dist distdir: update-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	for file in $$dists; do \
	  ln $(srcdir)/$$file $(distdir) 2> /dev/null \
	    || cp -p $(srcdir)/$$file $(distdir); \
	done

update-po: Makefile
	$(MAKE) $(GETTEXT_PACKAGE).pot
	@catalogs='$(CATALOGS)'; \
	INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) ; \
	export INTLTOOL_EXTRACT ; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
          if [ ! -f $$lang.po ] ; then \
             cp $(srcdir)/$$lang.po . ; \
          fi ; \
	  cp $$lang.po $$lang.old.po; \
	  echo $(MSGMERGE) $$lang; \
	  if   $(MSGMERGE) $$lang >/dev/null 2>&1; then \
	    rm -f $$lang.old.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.po; \
	    mv $$lang.old.po $$lang.po; \
	  fi; \
	done

.po: Makefile
	$(MAKE)  $(PACKAGE).pot;
	INTLTOOL_EXTRACT=$(INTLTOOL_EXTRACT) ; \
	export INTLTOOL_EXTRACT ; \
	echo; printf "$*: "; \
	if $(MSGMERGE) $*; then \
	  rm -f $*.old.po; \
	  else \
	    echo "msgmerge for * failed!"; \
	    mv $*.old.po $*.po; \
	fi; \
	msgfmt --statistics $*.po; echo;


# POTFILES is created from POTFILES.in by stripping comments, empty lines
# and Intltool tags (enclosed in square brackets), and appending a full
# relative path to them
POTFILES: POTFILES.in
	( if test 'x$(srcdir)' != 'x.'; then \
	    posrcprefix='$(top_srcdir)/'; \
	  else \
	    posrcprefix="../"; \
	  fi; \
	  rm -f $@-t $@ \
	    && (sed -e '/^#/d' 						\
		    -e "s/^\[.*\] +//" 					\
		    -e '/^[ 	]*$$/d' 				\
		    -e "s@.*@	$$posrcprefix& \\\\@" < $@.in		\
		| sed -e '$$s/\\$$//') > $@-t \
	    && chmod a-w $@-t \
	    && mv $@-t $@ )

Makefile: stamp-it
	@if test ! -f $@; then \
		rm -f stamp-it; \
		$(MAKE) stamp-it; \
	fi

stamp-it: Makefile.in.in ../config.status POTFILES
	cd .. \
	  && CONFIG_FILES=$(subdir)/Makefile.in CONFIG_HEADERS= \
	       $(SHELL) ./config.status

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
