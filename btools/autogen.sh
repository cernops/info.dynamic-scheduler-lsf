make distclean
find . -name \*~ -exec rm -f \{} \;
rm -rf autom4te.cache
rm -f depcomp install-sh missing mkinstalldirs
aclocal
automake --add-missing
autoconf --include=/opt/lsf/7.0  --include=/opt/lsf/6.0  --include=/opt/lsf


