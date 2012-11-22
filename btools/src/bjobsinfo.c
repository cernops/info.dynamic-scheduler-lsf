/* 
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 
   File:      $Id: bjobsinfo.c,v 1.7 2012/06/15 09:15:27 uschwick Exp $
   ChangeLog: 
              $Log: bjobsinfo.c,v $
              Revision 1.7  2012/06/15 09:15:27  uschwick
              only query for a list of submission hosts instead of all hosts in the system now

              Revision 1.7  2009/10/19 13:39:21  uschwick
              add support for type==any in bjobsinfo

              Revision 1.6  2007/11/22 08:43:45  uschwick
              bug fix scanning target type

              Revision 1.5  2007/02/05 15:46:27  uschwick
              added new field: target LSF host type

              Revision 1.4  2006/12/19 14:05:56  uschwick
              output additional information in bjobsinfo

              Revision 1.3  2006/11/18 11:24:44  uschwick
              added bjobsinfo man page to CVS

              Revision 1.2  2006/11/12 16:17:10  uschwick
              added bhostsinfo and bqueuesinfo tools

*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <lsf/lsbatch.h>
#include <unistd.h>

#include "bjobsinfo.h"

#define MAXCH 200
#define MAXSUBHOSTS 1000

JobInfo * JobInfoList;
char * submitHosts[MAXCH];
int numSubmitHosts = 0;


int AddJob(
	   char* jobId, 
	   char* user, 
	   char* state, 
	   char* queue, 
	   unsigned int submitTime, 
	   unsigned int startTime, 
	   unsigned int endTime, 
	   int cpuLimit, 
	   int runLimit, 
	   char* fromHost, 
	   char* lsfState, 
	   unsigned int exitStatus, 
	   unsigned int exitInfo,
	   char* resources,
	   char* execHosts 
	   )
{
  /* create a new entry */
  struct JobInfo* ptr;
  int code;

  ptr = (struct JobInfo*) malloc(sizeof(JobInfo));
  if (ptr != NULL) {
    /* fill it */
    strncpy(ptr->jobId,jobId,MAXSTRLEN);
    strncpy(ptr->user,user,MAXSTRLEN);
    strncpy(ptr->state,state,MAXSTRLEN);
    strncpy(ptr->queue,queue,MAXSTRLEN);
    ptr->submitTime = submitTime;
    ptr->startTime = startTime;
    ptr->endTime = endTime;
    ptr->cpuLimit = cpuLimit;
    ptr->runLimit = runLimit;
    strncpy(ptr->fromHost,fromHost,MAXSTRLEN);
    strncpy(ptr->lsfState,lsfState,MAXSTRLEN);
    ptr->exitStatus = exitStatus;
    ptr->exitInfo = exitInfo;
    strncpy(ptr->resources,resources,1024);
    strncpy(ptr->execHosts,execHosts,20000);
    ptr->next = NULL;

    /* add it to the list */
    if (NULL == JobInfoList){
      JobInfoList = ptr;
      //printf("defining root\n");      
    } else {
      ptr->next = (struct JobInfo*)JobInfoList;
      JobInfoList = ptr;
      //printf("preprend entry\n");
    }
    code = 0;
  } else {
    code = 1;
  }
  return(code);
}

int AddSubmissionHost(char * hostname){
  int exists = 0;
  int i = 0;
  /* check if we know this host already */
  while (i<numSubmitHosts && 0 == exists){
    if (strcmp(submitHosts[i],hostname)) exists = 1;
    i++;
  }
  if (0 == exists){
    submitHosts[numSubmitHosts] = (char *) malloc(MAXCH+1);
    strncpy(submitHosts[numSubmitHosts],hostname,MAXCH);
    numSubmitHosts ++;
    submitHosts[numSubmitHosts] = NULL;
  }
  if (numSubmitHosts < MAXSUBHOSTS){
    return 0;
  } else {
    printf("ERROR: too many submission hosts. Increase the maximum of %d\n Aborting.\n",MAXSUBHOSTS);
    exit(1);
  }
}


int main(int argc, char* argv[]){

  static int called = 0;
  int njobs = 0;
  int i;
  int j;
  int n;
  char state[10];
  char type[20];
  char lsfstate[15];
  char * fromHost;
  char * resources;
  char * token;
  int more; 
  int numHosts  = 0;
  char queue[MAXCH];
  char execHosts[20001];

  struct hostInfo*   hInfo;
  struct jobInfoEnt* jobInfoPtr;
  JobInfo* ptr;

  int AllJobs = CUR_JOB;

  int opt;

  /* initialize lists */
  JobInfoList = NULL;

  /* read command line options */ 

  strcpy(queue,"");
  while ((opt = getopt(argc,argv,"haq:")) != -1){
    switch(opt) {
    case 'h':
      printf("Usage:\n");
      printf("%s [-a] [-h] [-q queue]\n",argv[0]);
      break;
    case 'a':
      AllJobs = ALL_JOB;
      break;
    case 'q':
      strncpy(queue,optarg,MAXCH);
      break;
    }
  }

  /* initialise LSBLIB and get conf env */

  if (!called){
    if (lsb_init(argv[0]) < 0){
      lsb_perror("lsb_init");
      return -1;
    } else {
      called=-1;
    }
  }
  
  /* get info on jobs */
  if (strlen(queue) > 0){
    njobs = lsb_openjobinfo(0, NULL, "all",queue , NULL, AllJobs);
  } else {
    njobs = lsb_openjobinfo(0, NULL, "all",NULL , NULL, AllJobs);
  }


  if (njobs < 0) {
    lsb_perror("lsb_openjobinfo");
    return -lsberrno;
  }
  
  i = 0;
  jobInfoPtr = lsb_readjobinfo(&more);
    while (jobInfoPtr!= NULL){
      i++;
      switch (jobInfoPtr->status) {
      case JOB_STAT_PEND:
	strcpy(lsfstate,"PEND");
	strcpy(state,"queued");
	break;
      case JOB_STAT_PSUSP: 
	strcpy(lsfstate,"PSUSP");
	strcpy(state,"queued");
	break;
      case JOB_STAT_WAIT:
	strcpy(lsfstate,"WAIT");
	strcpy(state,"queued");
	break;
      case JOB_STAT_RUN:
	strcpy(lsfstate,"RUN");
	strcpy(state,"running");
	break;
      case JOB_STAT_SSUSP:
	strcpy(lsfstate,"SSUSP");
	strcpy(state,"running");
	break;
      case JOB_STAT_USUSP:
	strcpy(lsfstate,"USUSP");
	strcpy(state,"running");
	break;
      case JOB_STAT_UNKWN:
	strcpy(lsfstate,"UNKWN");
	strcpy(state,"running");
	break;
      case JOB_STAT_EXIT:
	strcpy(lsfstate,"EXIT");
	strcpy(state,"done");
	break;
      case JOB_STAT_DONE:
	strcpy(lsfstate,"DONE");
	strcpy(state,"done");
	break;
      case JOB_STAT_PDONE:
	strcpy(lsfstate,"PDONE");
	strcpy(state,"done");
	break;
      case JOB_STAT_PERR: 
	strcpy(lsfstate,"PERR");
	strcpy(state,"done");
	break;
	/* map all other states to "done" which may be wrong */ 
      default:
	strcpy(lsfstate,"DONE");
	strcpy(state,"done");
	break;      
      }
      
      resources = jobInfoPtr->submit.resReq;

      n = jobInfoPtr-> numExHosts;
      strcpy(execHosts,"");
      if ( n>0 ) {
	for (i=0;i<n;i++){
	  strcat(execHosts,jobInfoPtr->exHosts[i]);
	  if (i+1<n) strcat(execHosts,",");
	}
      } else {
	strcat(execHosts,"-");
      } 
      
      
      if (0 != AddJob(
		      lsb_jobid2str(jobInfoPtr->jobId),
		      jobInfoPtr->user,
		      state,
		      jobInfoPtr->submit.queue,
		      (unsigned int)jobInfoPtr->submitTime,
		      (unsigned int)jobInfoPtr->startTime,
		      (unsigned int)jobInfoPtr->endTime,
		      jobInfoPtr->submit.rLimits[LSF_RLIMIT_CPU],
		      jobInfoPtr->submit.rLimits[LSF_RLIMIT_RUN],
		      jobInfoPtr->fromHost,
		      lsfstate,
		      (unsigned int)jobInfoPtr->exitStatus,
		      (unsigned int)jobInfoPtr->exitInfo,
		      resources,
		      execHosts
		      )
	  ){
	printf("Failed to add job");
	return 1;
      };

      jobInfoPtr = lsb_readjobinfo(&more);
    }
  lsb_closejobinfo();

  /* get additional info on the submission hosts */
  hInfo = ls_gethostinfo(NULL,&numHosts,submitHosts,numSubmitHosts,ALL_CLUSTERS);
  if (hInfo == NULL){
    lsb_perror("bhostsinfo: ls_gethostinfo() failed");
    exit(-1);
  }

  /* now loop over all recorded jobs */  
  ptr = JobInfoList;
  while (NULL != ptr){
    /* determine the target execution host type for pending jobs */
    /* first analyse user resource string requirements if present */
    if (0<strlen(ptr->resources)){
      if ((token = strstr(ptr->resources,"type")) != NULL){
	if (sscanf(token,"type==%[0-9a-zA-Z_-]",ptr->resources)){
	  strncpy(type,ptr->resources,20);
	}
	if (sscanf(token,"type=%[0-9a-zA-Z_-]",ptr->resources)){
	  strncpy(type,ptr->resources,20);
	}
      }
    }
    
    /* first check if a type has been requested */
    fromHost  = ptr->fromHost;
    type[0] = 0;
    /*
      no type requested, so we get it from the submission host type 
      NOTE: if the type of the submission host has changed since the job has been submitted 
      the returned information is WRONG
    */
    /* try to get it from the submission host type */
    if (0 == strlen(type) || 0 == strcmp(type,"any")){
      for (j=0;j<numHosts;j++){
	if (0 == strcmp(fromHost,hInfo[j].hostName)){
	  strncpy(type,hInfo[j].hostType,20);
	}
      }
      if (0 == strlen(type)){
	/* Failed to get host type for submission host */
	strcpy(type,"UNKNWN");
      }
    }
    
    
    /* 
       Finally print the results
    */    
    printf("%s %s %s %s %10u %10u %10u %10d %10d %10s %10s %15s %10u %10u %s\n",
	   ptr->jobId,
	   ptr->user,
	   ptr->state,
	   ptr->queue,
	   ptr->submitTime,
	   ptr->startTime,
	   ptr->endTime,
	   ptr->cpuLimit,
	   ptr->runLimit,
	   ptr->fromHost,
	   ptr->lsfState,
	   type,
	   ptr->exitStatus,
	   ptr->exitInfo,
	   ptr->execHosts
	   );
    ptr = ptr->next;
  }
  return (0);
}
