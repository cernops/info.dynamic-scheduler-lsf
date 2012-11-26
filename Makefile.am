SUBDIRS = perl src doc man
PACKAGE = info-dynamic-scheduler-lsf
VERSION = $(shell cat info-dynamic-scheduler-lsf.spec| grep "Version:"| cut -f 2 -d " ")
SOURCES=aclocal.m4 btools install-sh Makefile.in README AUTHORS configure src doc man autogen.sh ChangeLog configure.in ${PACKAGE}.spec Makefile.am   NEWS  TODO autom4te.cache COPYING INSTALL perl btools.src.tgz

all:	configure
####################################################################
# Prepare
####################################################################
prepare:
	@mkdir -p build/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

dist: prepare
	@tar  --gzip -cf btools.src.tgz btools/
	@tar  --gzip -cf build/${PACKAGE}-${VERSION}.tar.gz ${SOURCES}

rpm: dist
	@rpmbuild --define "_topdir $(CURDIR)/build/" -ta build/${PACKAGE}-${VERSION}.tar.gz

sources: dist
	@rpmbuild --define "_topdir $(CURDIR)/build/" -ts build/${PACKAGE}-${VERSION}.tar.gz

srpm: sources

clean:
	rm -f btools.src.tgz
	rm -rf build

distclean: clean
####################################################################
# Documentation
####################################################################

doc: man html

man: prepare

html: prepare
