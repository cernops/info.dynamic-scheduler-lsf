SUBDIRS = src doc
PACKAGE = btools
VERSION = $(shell cat btools_spec.template| grep "Version:"| cut -f 2 -d " ")
dist:
	cp -p  btools_spec.template btools.spec
	tar --exclude CVS --exclude "*~" --exclude "#*" -zcf /tmp/btools-${VERSION}.tar.gz *

rpm: dist
	rpmbuild -ta /tmp/btools-${VERSION}.tar.gz

sources: dist
	rpmbuild -ts /tmp/btools-${VERSION}.tar.gz

clean:
	rm -f  /tmp/btools-${VERSION}.tar.gz
