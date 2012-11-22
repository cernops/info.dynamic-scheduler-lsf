package   EGI::CommandProxyTools;
require   Exporter;
@ISA    = qw(Exporter);
@EXPORT = qw(IniCommandProxyTools CommandProxy ReadInfoDynamicLsfConf WriteFileLock ReadFileLock RunCommand QueryCeType PrintDebug $CEProductionState $CEDefaultState $schedCycle @volist %VO2LSF %LSF2VO %VO2QUEUE %QUEUE2VO $LRMSInfoFile @LastCommandOutput @LsfLocalQueues $CELSFType $CacheID $GlueFormat $ReportWhichGroup $CPUScalingReferenceSI00 $glExec $Glue1StaticFileComp $Glue2StaticFileComp $Glue2StaticFileShare $binPath $MapFile $Debug DEBUG);

$VERSION = '2.2.1-1';
$ABSTRACT = "perl module to enable caching";
use strict;
use diagnostics;
use Date::Parse;
use File::Basename;
use Sys::Hostname;

# Global stuff
# debugging 
our $Debug = 0;

# reference to the last command's output
our @LastCommandOutput;

# location of configuration file
our $InfoDynamicLsfConfFile="/etc/lrms/lsf.conf";
our $CEDefaultState = "Default";
our $CEProductionState = "Production";
$InfoDynamicLsfConfFile=$1 if ((defined $ENV{"InfoDynamicLsfConfFile"}) && ($ENV{"InfoDynamicLsfConfFile"} =~ /^(\/[\/\d\w\_]+)$/));
 
# path to LRMS results
our $LRMSInfoFile;

# The list of VOs to be reported
our @volist;

# The user specified mapping of the VOs to the local batch system
our %VO2LSF;
our %LSF2VO; # reverse map, not allways defined!

# mapping of vo to their VO based queue 
our %VO2QUEUE;
our %QUEUE2VO;

# list of unexported queues that can be used by the VOs
our @LsfLocalQueues;

# scheduling interval
our  $schedCycle=120; 

# the LSF type of the CE we are runnning on
our $CELSFType;

# addendum to cache file name. Used to determine which cache files should be shared 
our $CacheID;

# print in format Glue1, Glue2 or both 
our $GlueFormat;

# Static input files
our $Glue1StaticFileComp;
our $Glue2StaticFileComp;
our $Glue2StaticFileShare;

# group to be reported in the lrm 
our $ReportWhichGroup;

# CE capabilities
our $CPUScalingReferenceSI00;
our $glExec = "yes";

# Path to LSF executables
our $binPath = "/usr/bin";
our $MapFile = "/etc/lrms/info-dynamic-scheduler.conf";

# seconds to wait between retries when running a command
my $WaitRetrySec  = 30; # wait between retry  

# maximal number of retries
my $MaxRetryCount = 10;

# settings for the proxy if used
my $verbose = 0;
my $UseCaching = 1;
my $RefreshEvery;
our %DefaultRefreshEvery = (
			   "default"      => 300,
			   "bugroups"     => 18000,
			   "lshosts"      => 18000,
			   "lsid"         => 180,
			   "bqueues"      => 60,
			   "bhosts"       => 10800, 
			   "bmgroup"      => 10800, 
			   "bjobs"        => 120,
			   "bjobsinfo"    => 120,
			   "bhostsinfo"    => 10800,
			   "blimit"    => 10800,
                           "lrmsinfoSLC5_64.txt" => 120,
			   );

my $VarFileLocation = "/var/cache/info-dynamic-lsf";
my %TimeOut;
$TimeOut{"default"} = 120;
my $UserCommandName;
my $UserCommandTimeout = $TimeOut{"default"};
my $UserCommand;
my $BaseName;
my $MyPid = $$;
my $MyHostname = `hostname`;
my $CacheFileName;
my $TmpFileName;
my $LockFileName;
my $ReadConfig=1;

sub PrintDebug($) {
    my $comment = shift;
    my $TimeStamp = time();
    print DEBUG "#DEBUG: $TimeStamp: $comment\n" if ($Debug);
}

sub cleanup {
    # clean up
    unlink $TmpFileName if (defined $TmpFileName && -e $TmpFileName);
    unlink $LockFileName if (defined $LockFileName && -e $LockFileName);
}

END {
    cleanup();
    close(DEBUG) if ($Debug);
}

sub IniCommandProxyTools($){
    $binPath = shift;
    $verbose = 1 if ($Debug);
    open(DEBUG,">/tmp/info_dynamic_lsf$$.log") if ($Debug);
    ReadInfoDynamicLsfConf();
    chomp($binPath);
    # we first have to check what we are running on
    QueryCeType($binPath);
}

sub RunCommand($) {
    my $command = shift;
    my $attempt_nr = 0;
    my $resultref;
    while ($attempt_nr < $MaxRetryCount){
	$resultref = CommandProxy("$command");
	last if ($resultref);
	sleep $WaitRetrySec;
    }
    die "failed to run $command" if (! @{$resultref});
    return ($resultref);
}

sub QueryCeType($) {
    # we get the CE type from LSF rather than the installed OS
    # this way it is possible to run on a SLC3 OS a CE that actually submits to SLC4 ...
    my $binPath = shift;
    my $hostname = hostname();
    $hostname = $1 if ($hostname =~ /^([\w\d\_]+)\./);
    my %hoststype;
    foreach my $line (@{RunCommand("$binPath/lshosts -w $hostname")}){
	my ($type) = (split /\s+/,$line)[1];
	$hoststype{$hostname} = $type;
    }
    $CELSFType = $hoststype{$hostname};
    #print "DEBUG: INI $CELSFType\n";
    return();
}

#
# check if there is a configuration file. If so, parse it
#

sub ReadInfoDynamicLsfConf()
{
    #print "DEBUG: ReadLCGInfoDynamicLsfConf $CELSFType\n";
    #
    # idea here: specify the mapping either hard coded in the form
    # vo = lsfgroup
    # or provide a bash like script that converts the vo into the LSF group
    # 
    $InfoDynamicLsfConfFile = $ENV{"InfoDynamicLsfConfFile"} if ($ENV{"InfoDynamicLsfConfFile"});
    undef $LRMSInfoFile;
    my $lsf_vo_map;
    my $lsf_qu_map;
    my $vo;
    # default : print it out
    $GlueFormat = "glue1";
    # setup default mapping
    foreach $vo (@volist){
	if (! defined $VO2LSF{$vo}){
	    $VO2LSF{$vo}=$vo;
	    $LSF2VO{$vo}=$vo;
	}
    } 
    if (-e $InfoDynamicLsfConfFile){
	PrintDebug("Reading $InfoDynamicLsfConfFile\n");
	if (open(CONF,$InfoDynamicLsfConfFile)){
	    while (<CONF>){
		next if /^\#/;
		next if /^\s*$/;
		chomp;
		if (/^GlueFormat\s*=\s*(.*)/i || /^outputformat\s*=\s*(.*)/i){
		    my $format = $1;
		    if ($format =~ /glue1/i || $format =~ /glue2/i || $format =~ /both/i){
			$GlueFormat = $format;
		    } else {
			print STDERR "FATAL: Unknown output format specified in config file:$format\n";
			exit(1);
		    }
		    next;
		}
		if (/(^\w+)_timeout\s*=\s*(\d+)/){
		    my $key = lc($1);
		    $TimeOut{$key} = $2;
		    PrintDebug("Command $key timout is set to $TimeOut{$key}\n");
		}
		#
		# for LRM: which group should be reported
		# 
		# GID  : the primary Unix GID which can be resolved  by yaim (from /etc/group and /etc/passwd)
		# LSF  : LSF group to which the user belongs
		# LSFR : top level LSF group to which the user belongs (bugroup -r)
		if (/ReportWhichGroup\s*=\s*(.*)/){
		    my $model = $1;
		    chomp $model;
		    $model =~ s/\"//g;
		    if ($model eq "GID" || $model eq "LSF" || $model eq "LSFR"){
			$ReportWhichGroup = $model;
			PrintDebug("Will report on $model\n");
		    }
		    next;
		}
		# this shall be a command or an external thing. 
		$lsf_vo_map = $1 if /LSF_GROUP_MAP_COMMAND\s*=\s*(.*)/;
		$lsf_qu_map = $1 if /LSF_QUEUE_MAP_COMMAND\s*=\s*(.*)/;
		# format  of hard coded mapping : vo=lsfgroup
		if (/^map\s+([\w\d\_\+\-]+)\s*\=\s*(\w[\w\d\_\+\-]+)/){
		    $VO2LSF{$1}=$2;
		    $LSF2VO{$2}=$1;
		    PrintDebug("Reading hard coded mapping $1 -> $2 \n");
		} 
		# CE default status
		$CEDefaultState = $1  if (/^LSF_CE_DEFAULT_STATUS\s*=\s*\"*(\w+)\"*/);
		PrintDebug("Default published state will be $CEDefaultState\n") if $CEDefaultState;
		# CE default production status (production or preproduction)
		$CEProductionState = $1 if (/^LSF_CE_PRODUCTION_STATUS\s*=\s*\"*(\w+)\"*/);
		PrintDebug("Default production state is $CEProductionState\n") if $CEProductionState;
		# where to store the LSRM information
		$LRMSInfoFile = $1 if (/^LSF_CE_LRMS_CACHE_LOC\s*=\s*\"*([\w\d\_\+\-\/]+)\"*/);
		PrintDebug("LRMS info file is at $LRMSInfoFile \n") if $LRMSInfoFile;
		# use caching or not
		if (/^UseCaching\s*=*\s*(\d+)/){
		    $UseCaching = $1;
		    next;
		}
		# where do the cache files go
		$VarFileLocation = $1 if /^CacheFileLoc\s*=*\s*\"*([\d\w\/\+\-\_]+)\"*/;
		# cache file refresh frequencies 
		$DefaultRefreshEvery{$1}=$2 if /^(\w+)\s*=*\s*(\d+)$/;
		@LsfLocalQueues=split /\s+/, $1 if /^LSF_LOCAL_QUEUES\s*\=\s*\"(.+?)\"*/;
		$CPUScalingReferenceSI00 = $1 if (/^CPUScalingReferenceSI00\s*=\s*(\d[\d\.]+)/);
		$glExec = lc($1)  if (/^glExec\s*=\s*([yYtToO]\w*)/);
		$Glue1StaticFileComp =  $1  if (/^glue1-static-file-CE\s*=\s*\"*([\w\d\_\+\-\/\.]+)\"*/);
		$Glue2StaticFileComp =  $1  if (/^glue2-static-file-computing-manager\s*=\s*\"*([\w\d\_\+\-\/\.]+)\"*/);
		$Glue2StaticFileShare = $1  if (/^glue2-static-file-computing-share\s*=\s*\"*([\w\d\_\+\-\/\.]+)\"*/);
		$Glue2StaticFileShare = $1  if (/^glue2-static-file-computing-share\s*=\s*\"*([\w\d\_\+\-\/\.]+)\"*/);
		$binPath = $1  if (/^binPath\s*=\s*\"*([\w\d\_\+\-\/\.]+)\"*/);
		$MapFile = $1  if (/^MapFile\s*=\s*\"*([\w\d\_\+\-\/\.]+)\"*/);
		$glExec = lc($1)  if (/^glExec\s*=\s*([yYtToO]\w*)/);
	    }
	    close CONF;
	    foreach my $command (keys %DefaultRefreshEvery){
		PrintDebug("Default caching time for $command is $DefaultRefreshEvery{$command}\n");
	    }
	    # remove back ticks from command if any
	    my $userid = `whoami`;
	    if ($userid ne "root"){
		foreach $vo (@volist){
		    if ($lsf_vo_map){
			$lsf_vo_map =~ s/\`//g;
			my $command = "vo=$vo;$lsf_vo_map";
			if (open(MAP,"$command|")){
			    while (<MAP>){
				chomp;
				if (/^(\w[\w\d\_\-]+)$/){
				    $VO2LSF{$vo}=$1;
				    $LSF2VO{$1}=$vo;
				}
			    }
			}
			close MAP;
		    }
		    PrintDebug("mapping $vo -> $VO2LSF{$vo}");
		    if ($lsf_qu_map){
			$lsf_qu_map =~ s/\`//g;
			my $command = "vo=$vo;$lsf_qu_map";
			if (open(MAP,"$command|")){
			    while (<MAP>){
				chomp;
				if (/^(\w[\w\d\_\-]+)$/){
				    $VO2QUEUE{$vo}=$1;
				    $QUEUE2VO{$1}=$vo;
				}
			    }
			}
			close MAP;
		    }
		    PrintDebug("mapping $vo -> $VO2QUEUE{$vo}") if (defined $vo && defined  $VO2QUEUE{$vo});
		}
	    }
	}
    }
    $ReadConfig = 0;
    QueryCeType($binPath) if ($binPath && ! $CELSFType);
    $LRMSInfoFile = $VarFileLocation if (! defined $LRMSInfoFile); # setup the default path
    $LRMSInfoFile =~ s/\/$//;
    $LRMSInfoFile .= "/lrmsinfo".$CELSFType.".txt";
    $schedCycle = $DefaultRefreshEvery{"bjobs"} if (defined  $DefaultRefreshEvery{"bjobs"});
}


#sub CreateVarFileLocation(){
#    if (! -d $VarFileLocation){
#	mkdir $VarFileLocation, 0775 || die "cannot create directory!";
#    }
#}

sub PidIsRunning($) {
    my $pid = shift;
    chomp $pid;
    # my $code = kill 0,$pid; # does not seem to work properly 
    #system("kill -0 $pid 2>&1 /dev/null");
    my $killcommand = "kill -0 $pid";
    my $code = 1;
    my $retry = 1;
    while(1){
	PrintDebug("Checking for process $pid retry $retry\n");
	if (defined open(KILL,"$killcommand 2>&1 |")){
	    while (<KILL>){
		$code = 0 if (/process/);
	    }
	    close KILL;
	    last;
	    
	} else {
	    $retry ++;
	    if ($retry < 10){
		PrintDebug("Cannot for a new process, retry $retry\n");
	    } else {
		die "cannot for process";
	    }
	}
    }
    
    if ($code){
	PrintDebug("Process $pid is still running: $code\n");
    } else {
	PrintDebug("Process $pid is gone: $code\n");	
    }
    return($code);
}

sub RunCommandTimeout {
    my $TimeOut =  shift;
    my $command = shift;
    my $childpid = 0;
    my $failed = 0;
    undef @LastCommandOutput;
    eval {
	local $SIG{ALRM} = sub { die "timeout\n" };
        PrintDebug("Running NOW \"$command\" with TIMEOUT $TimeOut");
	alarm $TimeOut;
	if ($childpid = open(COMM," $command 2>&1|")){
	    while (<COMM>){
		PrintDebug("COMM: $_");
		chomp;
		push @LastCommandOutput,$_;
	    }
	    close(COMM);
	    $childpid = 0;
	} else {
	    PrintDebug("COMM: failed to fork child process");
	};
	alarm 0;
    };
    my $res=$@;
    PrintDebug("RES: $res");
    if ($res eq "timeout\n") {
	if ($childpid>0){
	    PrintDebug("Command timed out. Sending term to last child $childpid");
	    kill "INT", $childpid; 
	    sleep 1;
	    kill "TERM", $childpid; 
	    alarm(0);
	} else {
	    PrintDebug("no last child found");
	}
	# make sure we do not return any truncated data
	undef @LastCommandOutput;
	$failed = 1;
    }
    # result is in global variable ...
    return ($failed);;
}

sub CheckCacheValidity {
    my $Now = time();
    my $Result = 0;
    my $Age;
    if (open(CACHE,$CacheFileName)){
	my $Oldtime = <CACHE>;
	chomp $Oldtime;
	$Age = $Now-$Oldtime;
	$Result = 1 if (($Oldtime =~ /^\d+$/) && ($Age<$RefreshEvery));
	close CACHE;
	PrintDebug("Checking Cache Validity: Age is $Age limit is $RefreshEvery Result is $Result\n");	
    } else {
	PrintDebug("No Cache file found.\n");	
    }
    return $Result;
}

sub CheckLock {
    my $LockFileName = shift;
    # read the existing lock file
    if (defined open(OLDLOCK,"$LockFileName")){
	my $oldpid = <OLDLOCK>;
	my $oldtimestamp = <OLDLOCK>;
	my $oldhostname;
	if (!($oldhostname = <OLDLOCK>)){
	    PrintDebug("lock file has no host information\n");
	    $oldhostname = "unknownhost";
	}
	close OLDLOCK;
	if ( defined $oldpid && defined $oldtimestamp && defined $oldhostname){
	    chomp $oldpid;
	    chomp $oldtimestamp;
	    chomp $oldhostname;
	    my $Current = time();
	    my $duration = $Current - $oldtimestamp;
	    PrintDebug("Lock file from process $oldpid time $oldtimestamp age $duration seconds host $oldhostname\n");
	    # check if the process is still running
	    if (($oldhostname =~ $MyHostname) && (PidIsRunning($oldpid))){
		# process is still active, lockfile from this host. Check time stamps
		PrintDebug("$oldpid is still running since $duration seconds\n");
		if (($duration)>300) {
		    PrintDebug("Removing stale lock file $LockFileName\n");
		    unlink $LockFileName;
		    # Todo: send an e-mail or something like that
		    return(0);
		} 
	    } else {
		if (($duration)>600) {
		    PrintDebug("Removing stale lock file $LockFileName from host $oldhostname\n");
		    unlink $LockFileName;
		    return(0);
		} else {
		    # do not refresh but return old cashed results
		    return(1);
		}
	    }
	}
    }
    return(0);
}

sub WaitLock($){
    my $LockFileName = shift;
    my $i = 0;
    while (-e $LockFileName){
	last if (0 == CheckLock($LockFileName));
	sleep 5;
	$i++;
	die "FATAL: File permanently locked. $LockFileName" if ($i > 200);
    }
}

sub CreateLock($){
    my $LockFileName = shift;
    my $i = 0;
    my $Current = time;
    PrintDebug("Creating lock file $LockFileName PID=$MyPid Timestamp=$Current\n");
    while (!-e $LockFileName){
	if (defined open(LOCK,">$LockFileName")){
	    print LOCK  $MyPid     . "\n";
	    print LOCK $Current    . "\n";
	    print LOCK $MyHostname . "\n";
	    close LOCK;
	    chmod 0664,$LockFileName;
	    sleep 1;
	} else {
	    my $error = $?;
	    $i++;
	    if ($i<20){
		PrintDebug("FATAL: cannot create lock file because \"$error\".\nRetry $i in 5sec\n");
		sleep 5;
	    } else {
		die("FATAL: cannot create lock file. Giving up.\n");
	    }
	}
    }
}

sub RefreshCache {
# 
# attempt to get a lock to refresh
#
    my $Current = time;
    my $res = 0;
    PrintDebug("Need to refresh cache\n");
    if (-e  $LockFileName ){
	PrintDebug("Cache is locked by $LockFileName\n");
	if (CheckLock($LockFileName)){
	    PrintDebug("Check lock returned 1\n");
	    return(1);
	} # return cached results while cache is being updated 
	# lock file has gone or was removed because it was out-of-date
	PrintDebug("Conditional recursive refresh cache\n");
	sleep(5); 
	$res = RefreshCache() if (! CheckCacheValidity());
    } elsif ( -e $CacheFileName  &&  ! -w $CacheFileName ) {
	# cache file is not writable. No point to try to recreate it.
	$res = 1;	
    } else {
	# create a new lock file
	my $i = 1;
	CreateLock($LockFileName) if (! (-e $LockFileName));
	# spawn the given command and write the output into the new cache file
	my $failed = RunCommandTimeout($UserCommandTimeout,"$UserCommand 2>/dev/null");
	# run the user command with a reasonable time out value
	if ((! $failed) && @LastCommandOutput){
	    unlink $TmpFileName if (-e $TmpFileName);
	    my $TimeStamp = time;
	    if (defined open(NEWCACHE,">$TmpFileName")){
		PrintDebug("Creating new cache file named $TmpFileName\n");
		# add time stamp as first line;
		print NEWCACHE $TimeStamp . "\n";
		foreach (@LastCommandOutput) {print NEWCACHE $_ . "\n";};
		close NEWCACHE;
		sleep 1; # wait for flush
	    } else {
		PrintDebug("Failed to open new cache file\n");
		unlink $LockFileName;
		return (1); # this would create an empty cache so exit !
	    }
	    # everything went ok, rename the file
	    chmod 0664,$TmpFileName;
	    PrintDebug("Attempt to replace old $CacheFileName by new cache $TmpFileName\n");
	    unlink $CacheFileName;
	    if (rename $TmpFileName,$CacheFileName){
		PrintDebug("Successfully created new cache file\n");
	    } else {
		PrintDebug("Failed to rename new cache file $?");
		$res = 1;
	    };
	    sleep 1; # flush
	    unlink $LockFileName;
	} else {
	    PrintDebug("Failed to run command\n");
	    $res = 1;
	    unlink $LockFileName;
	}
    }
    return($res);
}

sub ReturnCacheContents{
    undef @LastCommandOutput;
    my $retry_count = 0;
    my $maxretry = 150;
    PrintDebug("Returning cached results\n");
    while ($retry_count < 150){
	undef @LastCommandOutput;
	if (defined open(CACHE,$CacheFileName)){
	    PrintDebug("Reading cache contents\n");
	    my $Oldtime = <CACHE>;
	    while (<CACHE>) {push @LastCommandOutput, $_;};
	    close CACHE;
	    PrintDebug("ReturnCacheContents $CacheFileName: original ref:".\@LastCommandOutput."\n");
	    return (\@LastCommandOutput);
	} else {
	    # hmmm,  wait a bit and retry
	    $retry_count ++;
	    PrintDebug("Retry number $retry_count\n");
	    sleep(5); # hope somebody else does the work for use
	    my $res = RefreshCache() if (! -e $CacheFileName); # no ? then we have to try ourself ...
	}
    }
    die "FATAL: cannot read cache results after 150 retries. Giving up";
    return (\@LastCommandOutput);
}

#
# main entry point starts here
#
sub CommandProxy($) {
    #
    # returns a reference to an array containing the results of the command
    #
    undef @LastCommandOutput;
    my $fullcommand = shift;
    chomp $fullcommand;
    $UserCommand = $fullcommand;
    if ($UseCaching){
	$UserCommandName = (split /\s+/,$fullcommand)[0]; # command with full path but without arguments
	$UserCommandTimeout = (defined $TimeOut{basename($UserCommandName)}) ? $TimeOut{basename($UserCommandName)} : $TimeOut{"default"};
	PrintDebug("Request to run \"$UserCommand\" $UserCommandName\n");
	if (defined $DefaultRefreshEvery{basename($UserCommandName)}){
	    $RefreshEvery = $DefaultRefreshEvery{basename($UserCommandName)};
	} else {
	    $RefreshEvery = $DefaultRefreshEvery{"default"};
	}
	$BaseName = $fullcommand;
	$BaseName =~ s/[\s\/\,\>\&]/_/g;
	$CELSFType = "_" if (!defined $CELSFType);
	$CacheID   = "_" if (!defined $CacheID);
	$CacheFileName = "$VarFileLocation/".$BaseName . "_". $CELSFType . $CacheID . ".cache";
	$TmpFileName   = $CacheFileName . $MyPid;
	$LockFileName  = $CacheFileName . ".lock";
	PrintDebug("\"$BaseName\",\"$CacheFileName\",\"$TmpFileName\"");

# override the default timeout 
	PrintDebug("Refresh time is $RefreshEvery seconds\n");
	#CreateVarFileLocation();
	if (! CheckCacheValidity){
	    # try to refresh the cache up to 5 times then return contents
	    my $i = 0;
	    while (RefreshCache() && $i < 5){
		sleep 5;# retry after 5 sec
		return (ReturnCacheContents()) if CheckCacheValidity;
		$i++;
	    };
	}
	return (ReturnCacheContents());
    }  else {
	if (defined open(COMMAND,"$UserCommand 2>&1| ")){
	    while (<COMMAND>){
		push @LastCommandOutput,$_;
	    };
	    close (COMMAND);
	} else {
	    die "failed to run $UserCommand $?";
	}
	return(\@LastCommandOutput);
    }

    
};

# simple input/output

sub WriteFileLock($\@){
    my $filename = shift;
    my $contents = shift;
    $RefreshEvery = (defined $DefaultRefreshEvery{$filename} )?  $DefaultRefreshEvery{$filename} : $DefaultRefreshEvery{"default"};
    $LockFileName = $filename.".lock";
    $CacheFileName = $filename.".cache";
    # is the file locked ? Wait for it to be removed
    WaitLock($LockFileName) if (-e $LockFileName);
    # no need to rewrite if still valid
    return if (CheckCacheValidity());
    # lock the file
    CreateLock($LockFileName);
    # write the file
    my $lines = 0;
    my $newname  = $filename . ".new";
    unlink $CacheFileName;
    if (open(FILE,">$CacheFileName")){
	print FILE time() . "\n";
	close FILE;
    } else {
	unlink $LockFileName;
	return(1);
    }
    if (open(FILE,">$filename.new")){
	foreach my $line (@{$contents}) {
	    $lines ++;
	    print "undef in $lines $line\n" if (! defined $line); 
	    print FILE $line;
	};
	close FILE;
	sleep 1; # wait for flush
	rename $newname, $filename;
	unlink $LockFileName;
    } else {
	# can't write hence remove lock and return without action
	unlink $LockFileName;
	return(1);
    }
    chmod 0664,$filename;

    return(0);
}

sub ReadFileLock($){
    # read a file with locking
    my $filename = shift;
    my @contents;
    if ( ! -e $filename ){
	$LockFileName = $filename.".lock";
	# is the file locked ? Wait for it to be removed
	WaitLock($LockFileName)if (-e $LockFileName);
    }
    # lock the file
    CreateLock($LockFileName);
    # read it
    if (defined open(FILE,"<$filename")){
	while (<FILE>){
	    push @contents, $_;
	}
	close FILE;
    }
    unlink $LockFileName;
    return(@contents);
}

1;
