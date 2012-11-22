SUBDIRS = src doc

rpm:
	cp -p  btools_spec.template btools.spec
	tar --exclude CVS --exclude "*~" --exclude "#*" -zcf /tmp/btools.src.tgz *
	rpmbuild -tb /tmp/btools.src.tgz
	rm -f /tmp/btools.src.tgz

