Summary: LSF information provider plugin
Name: info-dynamic-scheduler-lsf
Version: 2.2.1
Vendor: CERN
Release: 1%{?dist}
License: ASL 2.0
Group: Applications/System
Source0: http://cern.ch/uschwick/software/info-dynamic-scheduler-lsf-%{version}.tar.gz
URL: https://github.com/schwicke/info.dynamic-scheduler-lsf
BuildArch: noarch
BuildRoot: %{_tmppath}/%{name}-%{version}-build
BuildRequires: perl
BuildRequires: autoconf
BuildRequires: automake
%{?el6:BuildRequires: perl-ExtUtils-MakeMaker}

%define debug_package %{nil}

%description
LSF information provider plugin 

%prep

%setup -q -c
./autogen.sh
./configure --prefix=$RPM_BUILD_ROOT/usr --mandir=$RPM_BUILD_ROOT/usr/share/man

%build
make 

%install
[ $RPM_BUILD_ROOT != / ] && rm -rf $RPM_BUILD_ROOT
make install 
rm -rf $RPM_BUILD_ROOT/usr/lib/perl5/vendor_perl/x86_64-linux-thread-multi
mkdir -p $RPM_BUILD_ROOT/usr/src/egi
install -m 644 btools.src.tgz ${RPM_BUILD_ROOT}/usr/src/egi

mkdir -p $RPM_BUILD_ROOT/var/lib
mkdir -p $RPM_BUILD_ROOT/var/cache/info-dynamic-lsf

%post
if [ ! -d /var/cache/info-dynamic-lsf ]; then
   /bin/mkdir -p /var/cache/info-dynamic-lsf
fi
/bin/chmod 1777 /var/cache/info-dynamic-lsf


%files
%defattr(-,root,root)
%{_usr}/libexec/info-dynamic-lsf
%{_usr}/libexec/info-shares-lsf
%{_usr}/libexec/lrmsinfo-lsf
%{_usr}/libexec/vomaxjobs-lsf
%{_usr}/libexec/runcmd
%{_usr}/lib/perl5/vendor_perl/EGI/CommandProxyTools.pm
%ghost%attr(755,root,root) /var/cache/info-dynamic-lsf
%config(noreplace)/etc/lrms/lsf.conf
%doc /usr/share/egi/doc/info.dynamic-scheduler-lsf/info-dynamic-lsf.txt
%doc %{_usr}/share/man/man2/lsf.conf.2.gz
%doc %{_usr}/share/man/man2/info-dynamic-lsf.2.gz
%doc %{_usr}/share/man/man2/info-shares-lsf.2.gz
%doc %{_usr}/share/man/man2/lrmsinfo-lsf.2.gz
%doc %{_usr}/share/man/man2/vomaxjobs-lsf.2.gz
%doc %{_usr}/share/man/man2/runcmd.2.gz

%clean
[ $RPM_BUILD_ROOT != / ] && rm -rf $RPM_BUILD_ROOT

%package btools
Summary: Additional LSF command line tools
Group: Applications/System
Autoreq: true

%description btools
Additional LSF command line tools 

%files btools
%defattr(-,root,root)
/usr/src/egi/btools.src.tgz
%doc /usr/share/egi/doc/info.dynamic-scheduler-lsf/btools.txt

%changelog
* Thu Nov 22 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.2.1-1
- make rpmlint happy
- move documentation for btools into the btools package
- remove glite compat package

* Mon Sep 03 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.2.0-1
- bug fix in runcmd

* Wed Jun 06 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.9-1
- support new version of bjobsinfo allowing to query grid queues only

* Wed Apr 04 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.8-4
- include path fix for SLC6

* Mon Mar 26 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.8-3
- spec file changes for EMI release

* Wed Mar 14 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.8-2
- spec file changes for mock builds

* Fri Feb 17 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.8-1
- add support for lsf exitStatus and exitInfo return by btools

* Thu Feb 09 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.7-1
- create cache directory in post scriplet if it does not exist
- make it a ghost 

* Wed Feb 08 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.6-1
- add support for bjobsinfo giving worker node information

* Thu Jan 19 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.5-2
- fix problem in naming of cache files for bhist calls done by BLAH
- fix tagging issue

* Thu Jan 12 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.4-1
- add possibility to set timeouts for commands 
- bug fix in setting the timeout

* Thu Jan 12 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.3-4
- reshuffle debugging code 

* Wed Jan 11 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.3-3
- patch for special characters in executed commands to be cached

* Tue Jan 10 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.3-2
- add protection in case a cache file is not writable for the account which runs the script
- add cache directory to rpm

* Mon Jan 09 2012 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.2-2
- all states should be lower case as glue2 is case sensitive
- maintain old schema for GLUE1

* Sat Dec 17 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-8
- correct glue2 publication fields, ref mail from M.Sgaravatto 17/12/2011

* Mon Dec 12 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-7
- patch for Savannah bug 77106 which also affects LSF (although differently)

* Sun Dec 11 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-6
- add missing perl module

* Sun Dec 11 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-5
- publish GLUE2EntityCreationTime for manager and each share in UTC

* Fri Dec 09 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-4
- additional link for static ldap files in compat package added
- change paths for glue2 in /etc/lrms/lsf.conf
- fix dublicate publishing of dynamic values for glue1

* Wed Dec 07 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-3
- update config file, and update paths to the new locations  

* Tue Dec 06 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-2
- fix format of man pages

* Tue Dec 06 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.1-1
- add man pages
- move executables to /usr/libexec
- bug fixes

* Tue Dec 06 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.0-12
- fix parsing of static glue2 input filex

* Tue Dec 06 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.0-11
- bug fixing
- change logic for glue2

* Mon Nov 28 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.1.0-1
- add support for GLUE2
- get rid of glite in naming conventions
- move config files to /etc/lrms/

* Fri Nov 11 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.0.23-1
- bug fix for the number of 9 in GlueCEPolicyMaxCPUTime

* Mon Apr 04 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.0.22-1
- be more consistend on reporting the nfree and nactive values. Be conservative as before. 

* Wed Mar 23 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.0.21-1
- add documentation
- add btools sources 

* Wed Mar 23 2011 Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 2.0.20-1 
- EGI compliant version
- remove hard coded dependency on edginfo 
- add gLite compatibility package