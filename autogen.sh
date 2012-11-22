make distclean
find . -name \*~ -exec rm -f \{} \;
rm -rf autom4te.cache
rm -f depcomp install-sh missing mkinstalldirs
aclocal
automake --add-missing
autoconf
./configure

