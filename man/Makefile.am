lsf.conf.man:
	pod2man --section=2 --release=info-dynamic-scheduler-lsf lsf.conf.2.pod > lsf.conf.man
info-dynamic-lsf.man:
	pod2man --section=2 --release=info-dynamic-scheduler-lsf info-dynamic-lsf.2.pod > info-dynamic-lsf.man
info-shares-lsf.man:
	pod2man --section=2 --release=info-dynamic-scheduler-lsf info-shares-lsf.2.pod > info-shares-lsf.man
lrmsinfo-lsf.man:
	pod2man --section=2 --release=info-dynamic-scheduler-lsf lrmsinfo-lsf.2.pod > lrmsinfo-lsf.man
vomaxjobs-lsf.man:
	pod2man --section=2 --release=info-dynamic-scheduler-lsf vomaxjobs-lsf.2.pod > vomaxjobs-lsf.man
runcmd.man:
	pod2man --section=2 --release=info-dynamic-scheduler-lsf runcmd.2.pod > runcmd.man
#runcmdas.man:
#	pod2man --section=2 --release=info-dynamic-scheduler-lsf runcmdas.2.pod > runcmdas.man
clean:
	rm -f *.man *~ 
distclean: clean
	rm -f  Makefile Makefile.in

man2_MANS =  lsf.conf.man info-dynamic-lsf.man info-shares-lsf.man lrmsinfo-lsf.man vomaxjobs-lsf.man runcmd.man
man5_MANS =  
EXTRA_DIST = $(man2_MANS) $(man5_MANS)

