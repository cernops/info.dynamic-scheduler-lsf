bjobsinfo.man:
	pod2man --section=1 --release=btools bjobsinfo.1.pod > bjobsinfo.man
bqueuesinfo.man:
	pod2man --section=1 --release=btools bqueuesinfo.1.pod > bqueuesinfo.man
bhostsinfo.man:
	pod2man --section=1 --release=btools bhostsinfo.1.pod > bhostsinfo.man
bacctinfo.man:
	pod2man --section=5 --release=btools bacctinfo.5.pod > bacctinfo.man
epoch2date.man:
	pod2man --section=5 --release=btools epoch2date.5.pod > epoch2date.man
date2epoch.man:
	pod2man --section=5 --release=btools date2epoch.5.pod > date2epoch.man
clean:
	rm -f *.man
distclean:
	rm -f *.man

man1_MANS =  bjobsinfo.man bqueuesinfo.man bhostsinfo.man
EXTRA_DIST = $(man1_MANS)
