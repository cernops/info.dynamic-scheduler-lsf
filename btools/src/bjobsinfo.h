/* 
   (C) 2012 CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 
   File:      $Id: bjobsinfo.h,v 1.1 2012/06/15 09:15:27 uschwick Exp $
   ChangeLog: 
*/

#define MAXSTRLEN 50

typedef struct JobInfo{
  char jobId[MAXSTRLEN+1];
  char user[MAXSTRLEN+1];
  char state[MAXSTRLEN+1];
  char queue[MAXSTRLEN+1];
  unsigned int submitTime;
  unsigned int startTime;
  unsigned int endTime;
  int cpuLimit;
  int runLimit;
  char fromHost[MAXSTRLEN+1];
  char lsfState[MAXSTRLEN+1];
  unsigned int exitStatus;
  unsigned int exitInfo;
  char resources[1025];
  char execHosts[20001];
  struct JobInfo * next;  
} JobInfo;


