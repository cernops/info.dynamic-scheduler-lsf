/*
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch>
   File:      $Id: bacctinfo.c,v 1.1 2011/03/24 14:49:16 uschwick Exp $
   ChangeLog:
              $Log: bacctinfo.c,v $
              Revision 1.1  2011/03/24 14:49:16  uschwick
              add btools toolsuite

              Revision 1.25  2008/01/27 10:32:13  uschwick
              bug fix in 'killed while pending' statistics

              Revision 1.24  2008/01/26 11:35:33  uschwick
              better protection against crazy CPU numbers

              Revision 1.23  2008/01/26 09:18:37  uschwick
              fixed casting bug in efficiency calculation

              Revision 1.22  2008/01/22 19:24:04  uschwick
              fixed floating point exception for non-running jobs

              Revision 1.21  2008/01/21 18:42:53  uschwick
              bugfixes in accounting tool

              Revision 1.20  2008/01/15 16:33:34  uschwick
              added a protection against crazy CPU times

              Revision 1.19  2008/01/15 09:55:36  uschwick
              fixed array bound error

              Revision 1.18  2007/08/29 16:28:40  dmicol
              Bugfixes.

              Revision 1.17  2007/08/24 17:39:42  dmicol
              Fixed the bug that affected the filtering by epoch when only one record
              was within the specified interval.

              Revision 1.16  2007/08/24 15:45:19  dmicol
              Config file lookup path changed.

              Revision 1.15  2007/08/24 08:40:32  dmicol
              Output compatibility with the per-user LSF Lemon sensor.

              Revision 1.14  2007/08/22 19:53:57  dmicol
              Bug fixes.

              Revision 1.13  2007/08/22 10:12:58  dmicol
              Added support for the --user all option.

              Revision 1.12  2007/08/20 16:00:48  dmicol
              Added filtering by user.

              Revision 1.11  2007/08/20 11:34:48  dmicol
              Added support for statistics by user.

              Revision 1.10  2007/08/15 16:43:59  uschwick
              synchronize last changes

              Revision 1.9  2007/02/05 15:46:27  uschwick
              added new field: target LSF host type

              Revision 1.8  2007/01/15 17:05:53  uschwick
              added two new special groups in bacctinfo: other and all

              Revision 1.7  2006/11/30 13:32:33  uschwick
              added cpu utilisation statistics

              Revision 1.6  2006/11/22 13:20:47  uschwick
              bugfix in print statement for x86_64

              Revision 1.5  2006/11/22 10:31:22  uschwick
              changed algorithm and added day-by-day statistics

              Revision 1.4  2006/11/18 11:24:44  uschwick
              added bjobsinfo man page to CVS

              Revision 1.3  2006/11/18 11:19:17  uschwick
              code cleanup

              Revision 1.2  2006/11/18 09:32:07  uschwick
              fixed NaNs

              Revision 1.1  2006/11/17 22:10:26  uschwick
              version 0.0.3 snapshot

   todo:     
	      - stepping: implement automatic day by day statistics 
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <lsf/lsf.h>
#include <lsf/lsbatch.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include "statistics.h"

#define LSF_GROUP_FILE "bacctinfo.conf"
#define MAXNUSER       50000
#define MAXUSERLENGTH  20
#define NUM_VALUES     20
#define SECONDSPERDAY  86400
#define MAXQNAMELEN    20

time_t StartEpoch;
time_t EndEpoch;
time_t Now;
char AllUsers[MAXGROUPS][MAXNUSER][MAXUSERLENGTH];
int AllUserIds[MAXNUSER][MAXGROUPS];
int nUsers[MAXGROUPS];

/* instantiate the root of the list */
struct value_list * value_list_root_all;
struct value_list * value_list_root_grid;
struct value_list * value_list_root_local;

int  n_lsfgroups;
int static_lsfgroups;
char * group_name;
char * user_name;

/* switches */
int filter;
int quiet;
int allUsers;

void usage(void){
  fprintf(stderr,"Usage: bacctinfo [--quiet] [--filter] [--start <start-epoch>] [--end <end-epoch>] [--user <lsf user | all>]\n");
  exit(22);
}

int AddLSFGroup(const char * lsf_group_name){
  int i, index;
  char group_names[8][100] = { "this period only; undef,other",
                               "this period only; running in GRID queue",
                               "this period only; submisson from other host",
                               "this period only; submission from any host",
                               "accumulated/total; undef,other",
                               "accumulated/total; running in GRID queue",
                               "accumulated/total; submisson from other host",
                               "accumulated/total; submission from any host" };

  for (i = 0; i < 8; i++){
    InitRunningSums(GetUser((index = AddUser(lsf_group_name, group_names[i]))), NUM_VALUES);
  }

  n_lsfgroups++;
  if (n_lsfgroups >= MAXGROUPS){
    fprintf(stderr,"Too many active users! Increase MAXGROUPS, current value is %d\n",MAXGROUPS);
    exit(1);
  }
  /* printf("DEBUG AddLSFGroup %s: index is %d, number of groups %d, returning %d\n",lsf_group_name,index,n_lsfgroups,index-7);*/
  return index - 7;
}

void RemoveLSFGroup(int group_id){
  int i;

  for (i = 0; i < 8; i++){
    DeleteUser(group_id);
  }

  n_lsfgroups--;
}

char * GetGroupNameInPasswdFileByUserName(char * user){
  struct passwd * pw;
  struct group * gr;

  pw = getpwnam(user);
  if (pw != NULL){
    gr = getgrgid(pw->pw_gid);
    if (gr != NULL){
      return gr->gr_name;
    }
  }

  return NULL;
}

int GetGroupIdInPasswdFileByUserId(int uid){
  struct passwd * pw;
  struct group * gr;

  pw = getpwuid(uid);
  if (pw != NULL){
    gr = getgrgid(pw->pw_gid);
    if (gr != NULL){
      return gr->gr_gid;
    }
  }

  return -1;
}

void ResolveGroup(char * user, int gid){
  struct groupInfoEnt * groupinfo;
  int numUsers;
  int i = 0;
  char * next;
  char * next1;
  char * target;
  struct passwd * pw;

  nUsers[gid] = 0;
  numUsers = 1;
  groupinfo = NULL;

  /* special group "all" catches all and everybody */
  memset(AllUsers[gid],0,MAXNUSER*MAXUSERLENGTH*sizeof(char));
  if (strcmp(user,"all")){
    if (strcmp(user,"other")){
      if (NULL != (groupinfo = lsb_usergrpinfo(&user,&numUsers,GRP_RECURSIVE))){
        for (i=0;i<numUsers;i++){
          next = groupinfo[i].memberList;
          target = AllUsers[gid][nUsers[gid]];
          next1 = next;
          while (NULL != next1){
            next1 = (char*) memccpy(target,next,0x20,MAXNUSER);
            if (target[strlen(target)-1] == 0x20){
              target[strlen(target)-1] = 0; /* remove trailing blank character */
            }
            pw = getpwnam(target);
            if (NULL != pw){
              AllUserIds[nUsers[gid]][gid] = pw->pw_uid;
            } else {
              AllUserIds[nUsers[gid]][gid] = 65534;
              if (!quiet) {
                fprintf(stderr,"mapping unknown user to nobody \"%s\" %d %d\n",target,(int)errno,(int)strlen(target));
              }
            }
            nUsers[gid] ++;
            next = next+(next1-target);
            if (abs(next-groupinfo[i].memberList)>=strlen(groupinfo[i].memberList)*sizeof(char)){
              next1 = NULL;
            }
            target = (char*)(&AllUsers[gid]) + MAXUSERLENGTH*nUsers[gid]*sizeof(char);
          };
        }
      } else {
        target = AllUsers[gid][nUsers[gid]];
        strncpy(target,user,MAXUSERLENGTH);
        pw = getpwnam(target);
        if (pw != NULL){
          AllUserIds[nUsers[gid]][gid] = pw->pw_uid;
          nUsers[gid]++;
        } else {
          if (!quiet) {
            fprintf(stderr, "Could not find user \"%s\" in password file\n",target);
          }
          exit(1);
        }
      }
    }
  }
}

void LoadLSFGroups(){
  int i;
  int number_of_groups;
  char lsf_group_name[20];
  char file_path[200];
  FILE * lsf_group_file;

  sprintf(file_path, "%s/%s", getenv("HOME"), LSF_GROUP_FILE);
  if ((lsf_group_file = fopen(file_path, "r")) == NULL){
    sprintf(file_path, "/etc/sysconfig/%s", LSF_GROUP_FILE);
    if ((lsf_group_file = fopen(file_path, "r")) == NULL){
      if (!quiet) { 
        fprintf(stderr, "Could not open %s\n", LSF_GROUP_FILE);
      }
      exit(1);
    }
  }

  number_of_groups = 0;
  do {
    if (fgetc(lsf_group_file) == '\n'){
      number_of_groups++;
    }
  } while (!feof(lsf_group_file));

  /* go back to the beginning of the file */
  fseek(lsf_group_file, 0, SEEK_SET);

  for (i = 0; i < number_of_groups; i++){
    fscanf(lsf_group_file, "%s\n", lsf_group_name);
    AddLSFGroup(lsf_group_name);
    ResolveGroup(lsf_group_name, i);
  }

  fclose(lsf_group_file);

  AddLSFGroup("other");
  AddLSFGroup("all");
  ResolveGroup("other", number_of_groups);
  ResolveGroup("all", number_of_groups + 1);
}

int GetGroupIdByUserName(const char * user_name) {
  int indx_lsfgroup, i;
  for (indx_lsfgroup = 0; indx_lsfgroup < n_lsfgroups; indx_lsfgroup++){
    for (i = 0; i < nUsers[indx_lsfgroup]; i++) {
      if (strcmp(user_name, AllUsers[indx_lsfgroup][i]) == 0) {
        return indx_lsfgroup;
      }
    }
  }

  return -1;
}

int GetGroupIdByUserId(int user_id) {
  int indx_lsfgroup, i;

  for (indx_lsfgroup = 0; indx_lsfgroup < n_lsfgroups; indx_lsfgroup++){
    for (i = 0; i < nUsers[indx_lsfgroup]; i++) {
      if (AllUserIds[i][indx_lsfgroup] == user_id) {
        return indx_lsfgroup;
      }
    }
  }

  return -1;
}

int GetPrimaryGroup(char * user){
  struct passwd * pw;
  pw = getpwnam(user);
  if (pw != NULL){
    return(pw->pw_gid);
  } else {
    if (!quiet) {
      fprintf(stderr, "Could not find user \"%s\" in password file\n",user);
    }
    return(-1);
  }
}

int requestedUser(char * userName,int uid, int gid){
  int i;
  if (0 == strcmp(GetUser(gid << 3)->user_name,"all")){
    /* matches all users */
    return(1);
  };
  if (0 == strcmp(GetUser(gid << 3)->user_name,"other")){
    /* matches all other users */
    if (GetGroupIdByUserId(uid) != -1) {
      return (0);
    } else {
      return (1);
    }
  } else {
    //printf("Debug: Select user: Group %s, uid %d gid %d\n",userName,uid,gid);
    /* check if this user has been selected or not */
    if (nUsers[gid] > 0){
      //printf("Debug: groupd contains %d users\n",nUsers[gid]);
      /* check if the user is in the list */
      for (i=0;i<nUsers[gid];i++){
	if (AllUserIds[i][gid] == uid){
	  //printf("Selecting user because he is in group %d, index %d\n",gid,i);
	  return(1);
	}
      }
      /* US remove this suspicios piece of code. It does not make sense I think
	 There must have been a reason for Daniel to add this. But what was it ?

      if (GetGroupIdInPasswdFileByUserId(uid) != gid) {
	//printf("Selecting user because of -what?\n");
        return (1);
      } else {
        return(0);
      }
      */
    } else {
      /* select all users */
      //printf("Selecting user anyway\n");
      return(1);
    }
  }
  return(0); /* never happens */
}

int GetFilter(){
  /* read stuff from STDIN and analyse it */
  /* store the finish time of the first and the last analysed job for each group */
  time_t start_count_epoch;
  time_t end_count_epoch;
  time_t Current;
  
  
  /* temporary local variables */
  int indx_lsfgroup;
  struct eventRec * record;
  struct jobFinishLog * jobFinish;
  time_t finishTime;
  int lineNumber;
  int printit;
  char * username;
      
  start_count_epoch = 0;
  end_count_epoch   = 0;
  Current = (StartEpoch > 0) ? StartEpoch : 0 ;
  
  /* read LSF accounting record from STDIN */
  record = lsb_geteventrec(stdin,&lineNumber);
  if (NULL == record){
    if (lsberrno == LSBE_EOF) return(0);
    lsb_perror("lsb_geteventrec failed");
    return(1);
  } 
  
  /* loop over records */
  while (NULL != record) {
    /* we are only interested in finished jobs */
    if (record->type == EVENT_JOB_FINISH){
      jobFinish = &(record->eventLog.jobFinishLog);
      finishTime = jobFinish->endTime;
      /* filter on requested time window */ 
      if (((0==StartEpoch)||(finishTime>=StartEpoch))&&((0==EndEpoch||finishTime<=EndEpoch))){
	
	/* remember the finish time of the first and the last job analysed */
	/* for grid/non-grid */
	if ( 0 == start_count_epoch){
	  start_count_epoch= finishTime;
	}
	end_count_epoch = finishTime;
	
	/* loop on groups */
	printit = 0;
	for (indx_lsfgroup = 0;indx_lsfgroup<n_lsfgroups;indx_lsfgroup++){
	  username = GetUser(indx_lsfgroup << 3)->user_name;
	  if (requestedUser(username,jobFinish->userId,indx_lsfgroup)){
	    printit = 1;
	  };
	}
	if (printit){
	  printf("%10s %5d %15s %10d %10d %10d\n",jobFinish->userName,GetPrimaryGroup(jobFinish->userName),jobFinish->queue,(int)jobFinish->submitTime,(int)jobFinish->startTime,(int)finishTime);
	  fflush(stdout);
	}
      }
    }
    record = lsb_geteventrec(stdin,&lineNumber);
  }

  return(0);
}

int GetStatistics(){

  /* read stuff from STDIN and analyse it */
  /* store the finish time of the first and the last analysed job for each group */
  time_t start_count_epoch;
  time_t end_count_epoch;
  time_t Current;
  

  /* a vector of values for each group */
  double value[MAXNUM];
  double total[MAXNUM];
  double mean[MAXNUM];
  double sd[MAXNUM];

  /* sum of totals, by group */
  double jobsph;
  double ksi2khr;
  double ksi2khc;
  
  /* integer statistics */
  int n_records;
  int done[MAXGROUPS*8];
  int exited[MAXGROUPS*8];
  int termadm[MAXGROUPS*8];
  int termusr[MAXGROUPS*8];
  int termcpu[MAXGROUPS*8];
  int termrun[MAXGROUPS*8];
  int termpnd[MAXGROUPS*8];

  /* temporary local variables */
  int i,j;
  int indx_lsfgroup;
  int indx_group;
  int indx_user;
  int indx_all;
  int is_grid;
  int non_grid;
  int grid_offset;
  struct eventRec * record;
  struct jobFinishLog * jobFinish;
  time_t finishTime;
  time_t pending;
  time_t running;
  int lineNumber;
  char queuename[MAXQNAMELEN+1];
  char queue[MAXQNAMELEN+1];
  int jstatus;
  int exitinfo;


  double cpufac;

  void PrintStatus(void){
    int i,k;
    /* total analysed time in hours */
    double time_measured;
    time_t epoch;

    /* loop again on groups to print out the results */
    for (indx_lsfgroup = allUsers ? static_lsfgroups : 0; indx_lsfgroup < n_lsfgroups; indx_lsfgroup++){
      //printf( "DEBUG Printing: indx_lsfgroup is now %d max %d\n",indx_lsfgroup, n_lsfgroups);
      for (k=0 ; k <= 1 ; k++){
	/* time analysed, in hours */
	//printf("DEBUG: times: start %d end %d\n",(int)start_count_epoch,(int)end_count_epoch);
        if (0 == k){
	  time_measured = (double)(end_count_epoch-start_count_epoch)/3600.0;
	  if (0 > time_measured){
            if (!quiet) {
	      fprintf(stderr,"no matching data found\n");
            }
	    exit(1);
	  }
	  epoch = start_count_epoch;
	} else {
	  time_measured = SECONDSPERDAY;
	  epoch = Current;
	}
        for (i = 1;i<4;i++){
	  indx_group = (k<<2) + (indx_lsfgroup << 3) + i;
	  /* do the statistics and print the results */
	  n_records = RunningSumsGetStatistics(GetUser(indx_group), total, mean, sd, NUM_VALUES);
	  jobsph     = (time_measured>0) ?  ((double)n_records)/time_measured : 0;
	  ksi2khc = total[15]/3600.0/24.0;
	  ksi2khr = total[14]/3600.0/24.0;

          if (allUsers) {
            int group_id = GetGroupIdByUserName(GetUser(indx_group)->user_name);
            if (group_id == -1) {
              printf("%s %s ", GetGroupNameInPasswdFileByUserName(GetUser(indx_group)->user_name), GetUser(indx_group)->user_name);
            } else {
              printf("%s %s ", GetUser(group_id << 3)->user_name, GetUser(indx_group)->user_name);
            }
          } else if (user_name != NULL) {
            printf("%s %s ", GetUser(indx_group)->user_name, user_name);
          } else {
            printf("%s ", GetUser(indx_group)->user_name);
          }

	  printf("%d %d %5.2f %6d %6d %6d %6d %6d %6d %6d %6d %13.2f %13.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f %10.2f\n",(k<<2)+i,(int)epoch,jobsph,
		 termpnd[indx_group],n_records,done[indx_group],exited[indx_group],
		 termadm[indx_group],termusr[indx_group],termcpu[indx_group],termrun[indx_group],
		 total[0],total[1],mean[0],mean[1],sd[0],sd[1],
		 ksi2khr,ksi2khc,
		 (time_measured>0) ? 24.0*ksi2khr/time_measured : 0,
		 (time_measured>0) ? 24.0*ksi2khc/time_measured : 0,
		 mean[7],mean[8],sd[7],sd[8],
		 mean[13],mean[14],sd[13],sd[14],
		 mean[3],mean[4],sd[3],sd[4],
		 mean[10],mean[11],sd[10],sd[11],
		 mean[2],
		 mean[19],sd[19]
		 );
	  fflush(stdout);
	}
      }   
    }

    /* reset values for the current period (k = 0) */
    printf("\n");
    for (indx_lsfgroup = allUsers ? static_lsfgroups : 0; indx_lsfgroup < n_lsfgroups; indx_lsfgroup++){
      for (i = 1;i<4;i++){
	indx_group =  (indx_lsfgroup << 3) + i;
	ResetRunningSums(GetUser(indx_group), NUM_VALUES);
      }
    }

  } /* end of PrintStatus */

  /* Main starts here. First initialize everything */
  for (i = 0;i < MAXGROUPS; i++){
    done[i]    = 0;
    exited[i]    = 0;
    termadm[i] = 0;
    termusr[i] = 0;
    termcpu[i] = 0;
    termrun[i] = 0;
    termpnd[i] = 0;
    if (i <= MAXNUM){
      total[i]   = 0.0;
      mean[i]    = 0.0;
      sd[i]      = 0.0;
    }
  }
  start_count_epoch = 0;
  end_count_epoch   = 0;
  Current = (StartEpoch > 0) ? StartEpoch : 0 ;

  /* read LSF accounting record from STDIN */
  record = lsb_geteventrec(stdin, &lineNumber);
  if (NULL == record){
    if (lsberrno == LSBE_EOF) return(0);
    lsb_perror("lsb_geteventrec failed");
    return(1);
  }

  void Process(struct jobFinishLog * jobFinish, int indx, int indx_all){
    /* filter jobs that were killed before they started */
    /*printf("DEBUG Process called with %d and %d\n",indx,indx_all);*/
    time_t cputime;
    time_t utilization;
    time_t utime;
    time_t stime;

    if ( 0 != (jobFinish->startTime)){
      pending = jobFinish->startTime - jobFinish->submitTime;
      running = finishTime - jobFinish->startTime;
      cpufac = (double)jobFinish->hostFactor;

      cputime =  jobFinish->cpuTime;
      if (cputime > 1.2*running) {
	if (!quiet) {
	  fprintf(stderr,"Found crazy CPU time value Run time %d, CPU time %d. Setting to default of 80 percent of runtime\n",(int)running,(int)cputime);
	  cputime=0.8*running;
	}	  
      }
      utime = jobFinish->lsfRusage.ru_utime;
      stime = jobFinish->lsfRusage.ru_stime;
      utilization = utime+stime;
      if (utilization > 1.2*running) {
	if (!quiet) {
	  fprintf(stderr,"Found crazy utilization time value Run time %d, util time %d. Setting to default of 80 percent of runtime\n",(int)running,(int)utilization);
	  utilization = 0.8*running;
	  utime = 0.4*running;
	  stime = 0.4*running;
	}	  
      }

      if ((pending <= 0)||(running <=0)) {
        if (!quiet) {
          fprintf(stderr,"Pending<=0: %d %d %d\n",(int)pending,(int)jobFinish->startTime, (int)jobFinish->submitTime);
          fprintf(stderr,"Running<=0: %d %d %d\n",(int)running, (int)finishTime, (int)jobFinish->startTime);
        }
      } else {

        exitinfo = jobFinish->exitInfo;
        jstatus  = jobFinish->jStatus;

        /* fill vector with general values for the current job */
        if ( jobFinish->lsfRusage.ru_inblock > 0){
          value[0]  = jobFinish->lsfRusage.ru_inblock; /* I/O */
        } else {
          value[0]  = 0;
        }
        if ( jobFinish->lsfRusage.ru_oublock > 0){
          value[1]  = jobFinish->lsfRusage.ru_oublock; /* I/O */
        } else {
          value[1]  = 0;
        }
        value[2]  = (double)(jobFinish->lsfRusage.ru_nswap); /* number of times the job was swapped out */
        value[3]  = (double)(jobFinish->maxRMem); /* maxim. Memory */
        value[4]  = (double)(jobFinish->maxRSwap); /* max. Swap */
        value[5]  = cpufac; /* interested in average only */
        value[6]  = ((double)utilization)/(double)running; /* cpu usage */

        /* real time values */
        value[7]   = (double)pending; /* real pending time */
        value[8]   = (double)running; /* real running time */
        value[9]   = (double)cputime; /* real CPU time */
        value[10]  = (double)utime; /* real user time */
        value[11]  = (double)stime; /* real system time */
        value[12]  = cputime; /* real CPU time */

        /* normalised time values */
        value[13]  = pending*cpufac; /* real pending time */
        value[14]  = running*cpufac; /* real running time */
        value[15]  = cputime*cpufac; /* real CPU time */
        value[16]  = (double)(utime)*cpufac; /* real user time */
        value[17]  = (double)(stime)*cpufac; /* real system time */
        value[18]  = cputime*cpufac; /* real CPU time */
        value[19]  = ((double)utilization)/(double)running; /* cpu utilisation */
	/* printf("DEBUG: Utilization: %f, Running: %f, Eff.%f\n",(double)utilization,(double)running,value[19]);*/

        /* fill in values for the groups both for cumulative and current */
        for (j=0;j<=1;j++){
	  //printf("Debug: GetUser with j=%d indx=%d\n",j,indx);
          RunningSumsAddValues(GetUser(indx+(j<<2)), value,NUM_VALUES);
          RunningSumsAddValues(GetUser(indx_all+(j<<2)), value,NUM_VALUES);
	  if (indx_all+(j<<2) > 8*MAXGROUPS){
	    printf("FATAL in Process: index out of bounds! %d >=%d",indx_all+(j<<2),MAXGROUPS);
	    exit(1);
	  }
          if (64 == jstatus){
            done[indx+(j<<2)]++;
            done[indx_all+(j<<2)]++;
          } else {
            exited[indx+(j<<2)]++;
            exited[indx_all+(j<<2)]++;
          };

          switch (exitinfo) {
            case TERM_RUNLIMIT:
              termrun[indx+(j<<2)] ++;
              termrun[indx_all+(j<<2)] ++;
              break;
            case TERM_CPULIMIT:
              termcpu[indx+(j<<2)] ++;
              termcpu[indx_all+(j<<2)] ++;
              break;
            case TERM_OWNER:
              termusr[indx+(j<<2)] ++;
              termusr[indx_all+(j<<2)] ++;
              break;
            case TERM_ADMIN:
              termadm[indx+(j<<2)] ++;
              termadm[indx_all+(j<<2)] ++;
              break;
            }
          }
        }
      } else {
        /* statistics for both current and cumulative */
        for (j=0;j<=1;j++){
          termpnd[indx+(j<<2)] ++;
          termpnd[indx_all+(j<<2)] ++;
      }
    }
  }

  int ProcessUser(struct jobFinishLog * jobFinish){
    int id;

    if ((id = GetUserIdWithoutOptions(jobFinish->userName)) == -1){
      id = AddLSFGroup(jobFinish->userName);
    }

    /* check from queue name if it is a grid job */
    strncpy(queue,jobFinish->queue,MAXQNAMELEN);
    is_grid = sscanf(queue,"grid_%s",queuename);
    non_grid = 1-is_grid;
    grid_offset = is_grid+(non_grid << 1);

    indx_user = id + grid_offset;
    indx_all   =  id + 3;

    Process(jobFinish, indx_user, indx_all);

    return 1; 
  }

  int ProcessGroup(char * username, struct jobFinishLog * jobFinish, int indx_lsfgroup){
    if (requestedUser(username,jobFinish->userId,indx_lsfgroup)){
      /* check from queue name if it is a grid job */
      strncpy(queue,jobFinish->queue,MAXQNAMELEN);
      is_grid = sscanf(queue,"grid_%s",queuename);
      non_grid = 1-is_grid;
      grid_offset = is_grid+(non_grid << 1);

      indx_group = (indx_lsfgroup << 3) + grid_offset;
      indx_all   =  (indx_lsfgroup << 3) + 3;

      //printf("Processing user %s index_group %d index_all %d, group index %d \n",username,indx_group,indx_all,indx_lsfgroup);
      Process(jobFinish, indx_group, indx_all);

      return 1;
    } else {
      return 0;
    }
  }

  /* GetStatistics starts here */

  /* loop over records */
  while (NULL != record) {
    /* we are only interested in finished jobs */
    if (record->type ==EVENT_JOB_FINISH){
      if (allUsers || user_name == NULL || strcmp(record->eventLog.jobFinishLog.userName, user_name) == 0){

        jobFinish = &(record->eventLog.jobFinishLog);
        finishTime = jobFinish->endTime;

        /* initialize the current timing counter with the time of the first job record if necessary */
        if (0 == Current){
	  Current = finishTime;
        }
        /* print out daily statistics */ 
        while (finishTime > (Current + SECONDSPERDAY)){
          start_count_epoch = Current;
          end_count_epoch = Current + SECONDSPERDAY;

 	  PrintStatus();
	  Current += SECONDSPERDAY;
        }

        /* filter on requested time window */ 
        if (((0==StartEpoch)||(finishTime>=StartEpoch))&&((0==EndEpoch||finishTime<=EndEpoch))){
	  /* remember the finish time of the first and the last job analysed */
	  /* for grid/non-grid */
	  if ( 0 == start_count_epoch){
	    start_count_epoch = finishTime;
	  }
	  end_count_epoch = finishTime;

          if (allUsers) {
	    //printf("DEBUG: calling ProcessUser\n");
            ProcessUser(jobFinish);
          } else if (user_name != NULL) {
	    //printf("DEBUG: calling ProcessGroup\n");
            ProcessGroup(group_name, jobFinish, 0);
          } else {
            /* loop on groups */
            for (indx_lsfgroup = allUsers ? static_lsfgroups : 0; indx_lsfgroup < n_lsfgroups; indx_lsfgroup++){
	      //printf("DEBUG: calling ProcessGroup for %s\n",GetUser(indx_lsfgroup << 3)->user_name);
              ProcessGroup(GetUser(indx_lsfgroup << 3)->user_name, jobFinish, indx_lsfgroup);
            }
          }
	}
      }
    }
    record = lsb_geteventrec(stdin,&lineNumber);
  }
  /* make sure that the last record is printed */
  PrintStatus();
  return(0);
}

void readOptions(int argc, char* argv[], char* lsfuser){
  int c;
  unsigned int epoch;
  /* defaults */

  /* filter jobs only */
  filter = 0;  

  /* by default, don't stay quiet */
  quiet = 0;

  allUsers = 0;

  /* read options */
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"start", 1, 0, 0},
      {"end", 1, 0, 0},
      {"user", 1, 0, 0},
      {"filter", 0, 0, 0},
      {"quiet", 0, 0, 0},
      {"help", 0, 0, 0},
      {0, 0, 0, 0}
    };
    
    c = getopt_long (argc, argv, "seu:h",
		     long_options, &option_index);
    if (c == -1)
      break;
    
    switch(c) {
    case '?':
      usage();
      break;
      
    case 0:
      if (optarg){
	epoch = 0;
	if (0 == strcmp(long_options[option_index].name,"start")){
	  if (0==sscanf(optarg,"%u",&epoch)){
	    usage();
	  }
	  StartEpoch = (time_t)epoch;
	  break;
	};
	if (0 == strcmp(long_options[option_index].name,"end")){
	  if (0==sscanf(optarg,"%u",&epoch)){
	    usage();
	  }
	  EndEpoch = (time_t)epoch;
	  break;
	};
	if (0 == strcmp(long_options[option_index].name,"user")){
	  strncpy(lsfuser,optarg,1023);
	}
	break;
      }
      if (0 == strcmp(long_options[option_index].name,"help")){
	usage();
	break;
      } 
      if (0 == strcmp(long_options[option_index].name,"filter")){
	filter = 1;
	break;
      }
      if (0 == strcmp(long_options[option_index].name,"quiet")){
        quiet = 1;
        break;
      }
    default:
      fprintf(stderr,"[ERROR] Unknown option %c\n",c);
      usage();
    }
  }
  if (StartEpoch > 0 && EndEpoch > 0 && StartEpoch >= EndEpoch){
    fprintf(stderr,"Start epoch must be lower than end epoch\n");
    exit(1);
  }
  if (StartEpoch > time(&Now)){
    fprintf(stderr,"Requested analysis starting time is in the future\n");
    exit(1);
  }

  return;
}

int main(int argc, char* argv[]){
  int indx_lsfgroup;
  char * accounting_file_name;
  char lsfuser[1024];
  char * gr_name;
  int res;

  memset(lsfuser,0,1024);
  StartEpoch = 0;
  EndEpoch=0;

  n_lsfgroups = 0;

  value_list_root_all   = NULL;
  value_list_root_grid  = NULL;
  value_list_root_local = NULL;

  accounting_file_name = argv[1];
  readOptions(argc,argv,lsfuser);

  if (lsb_init(argv[0])<0) {
    lsb_perror("lsb_init failed");
    exit(1);
  }

  /* initialise groups */
  LoadLSFGroups();
  static_lsfgroups = n_lsfgroups;

  /* first resolve the given user or group if given */
  if (strlen(lsfuser)>0){
    if (strcmp(lsfuser, "all") == 0){
      allUsers = 1;
    } else {
      int user_indx_group;

      group_name = NULL;
      if ((user_indx_group = GetGroupIdByUserName(lsfuser)) == -1) {
        if ((gr_name = GetGroupNameInPasswdFileByUserName(lsfuser)) != NULL){
          group_name = (char *) malloc((strlen(gr_name) + 1) * sizeof(char));
          strcpy(group_name, gr_name);
          user_indx_group = AddLSFGroup(group_name) >> 3;
        } else {
          if (!quiet) {
            fprintf(stderr, "User \"%s\" not in password file\n", lsfuser);
          }
          exit(1);
        }
      } else {
        group_name = (char *) malloc((strlen(GetUser(user_indx_group << 3)->user_name) + 1) * sizeof(char));
        strcpy(group_name, GetUser(user_indx_group << 3)->user_name);
      }

      for (indx_lsfgroup = n_lsfgroups - 1; indx_lsfgroup >= 0; indx_lsfgroup--) {
        if (indx_lsfgroup != user_indx_group){
          RemoveLSFGroup(indx_lsfgroup << 3);
        }
      }
    }

    user_name = (char *) malloc((strlen(lsfuser) + 1) * sizeof(char));
    strcpy(user_name, lsfuser);
  } else {
    /* no user has been specified */
    user_name = NULL;
    group_name = NULL;
  }

  if (filter){
    res = GetFilter();
  } else {
    res = GetStatistics();
  }

  if (user_name != NULL){
    free(user_name);
    user_name = NULL;
  }
  if (group_name != NULL){
    free(group_name);
    group_name = NULL;
  }

  DeleteUsers();

  exit(res);
}
