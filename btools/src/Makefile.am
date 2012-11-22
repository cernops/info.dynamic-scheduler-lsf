bin_PROGRAMS        = bjobsinfo bqueuesinfo bhostsinfo
bjobsinfo_SOURCES   = bjobsinfo.c
bqueuesinfo_SOURCES = bqueuesinfo.c
bhostsinfo_SOURCES  = bhostsinfo.c

AM_LDFLAGS = 

INCLUDES = -I/usr/local/lsf/include -I/usr/lsf/include -I../include
LDADD = -L/usr/local/lsf/lib -L/usr/lsf/lib -lbat -llsf -lnsl -lm -ldl $(INTLLIBS) 

