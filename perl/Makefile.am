all:
	cp -p EGI-CommandProxyTools/lib/EGI/CommandProxyTools.pm ./EGI-CommandProxyTools/blib/lib/EGI/CommandProxyTools.pm
	cd EGI-CommandProxyTools && perl Makefile.PL LIB=${prefix}/lib/perl5/vendor_perl && make 
install: all
	cd EGI-CommandProxyTools && make install 
