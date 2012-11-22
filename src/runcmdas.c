#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <syslog.h>
#include <pwd.h>

/* global variables */
char APPLICATION[30] = "/usr/libexec/runcmd";
char RUNAS[10] = "ldap";
struct passwd* entry;

int main(int argc, const char *argv[]){
  uid_t uid;
  uid_t euid;
  char command[2001];
  int i,j;
  
  strncat(command, APPLICATION, 20);
  j=2000-strlen(command);
  for (i=1; i<argc; i++){
    if (j>0){
      strncat(command," ",j);
      j = j -1;
      strncat(command, argv[i],j);
      j = j - strlen(argv[i]);
    }
  }
  syslog(LOG_INFO|LOG_USER,command);
  
  uid = getuid();
  euid = geteuid();
  if (0 == euid && 0==uid){
    entry = getpwnam(RUNAS);
    setuid(entry->pw_uid);
    setgid(entry->pw_gid);
  } else {
    syslog(LOG_WARNING|LOG_USER,strcat("Execution may fail for ", command));
  }
  if (execvp(APPLICATION,(char **)argv)) {
    fprintf(stderr,"Sorry, I failed to execute command %s. Error code is %d\n","/usr/libexec/runcmd",errno);
    return(1);
  }
  return 0;
}

