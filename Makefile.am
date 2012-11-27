SUBDIRS = perl src doc man
PACKAGE = info-dynamic-scheduler-lsf
VERSION = $(shell cat info-dynamic-scheduler-lsf.spec| grep "Version:"| cut -f 2 -d " ")
SOURCES=aclocal.m4 btools README AUTHORS configure src doc man autogen.sh ChangeLog configure.in ${PACKAGE}.spec Makefile.am   NEWS  TODO autom4te.cache COPYING INSTALL perl btools.src.tgz

####################################################################
# Prepare
####################################################################
all: dist

dist: 
	@tar  --gzip -cf btools.src.tgz btools/
	@tar  --gzip -cf ${PACKAGE}-${VERSION}.tar.gz ${SOURCES}

rpm: dist
	@rpmbuild -ta build/${PACKAGE}-${VERSION}.tar.gz

sources: dist

srpm: 
	@rpmbuild -ts build/${PACKAGE}-${VERSION}.tar.gz


clean:
	rm -f btools.src.tgz ${PACKAGE}-${VERSION}.tar.gz
	rm -rf build

distclean: clean
####################################################################
# Documentation
####################################################################

doc: man html

man: prepare

html: prepare
