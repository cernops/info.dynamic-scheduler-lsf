/* 
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 
   File:      $Id: bqueuesinfo.c,v 1.1 2011/03/24 14:49:16 uschwick Exp $
   ChangeLog: 
              $Log: bqueuesinfo.c,v $
              Revision 1.1  2011/03/24 14:49:16  uschwick
              add btools toolsuite

              Revision 1.3  2006/11/18 11:24:44  uschwick
              added bjobsinfo man page to CVS

              Revision 1.2  2006/11/17 22:10:26  uschwick
              version 0.0.3 snapshot

              Revision 1.1  2006/11/12 16:17:10  uschwick
              added bhostsinfo and bqueuesinfo tools

 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <lsf/lsbatch.h>
int main(int argc, char* argv[]){
  struct queueInfoEnt *qInfo;
  
  char * host = NULL;
  char * user = NULL;
  char * queues = NULL;
  int options = 0;
  int numQueues = 0;
  int i;

  if (lsb_init(argv[0]) < 0){
    lsb_perror("bqueuesinfo: lsb_init() failed");
    exit (-1);
  }
  
  /* query LSF */
  qInfo = lsb_queueinfo(&queues,&numQueues,host,user,options);
  if (qInfo == NULL){
    lsb_perror("bqueuesinfo: lsb_queueinfo() failed");
    exit(-1);
  }
  for (i=0;i<numQueues;i++){
    printf("%s %d %d %d %d %d %d %d %d\n",qInfo[i].queue,
	   qInfo[i].priority,
	   qInfo[i].qStatus,
	   qInfo[i].maxJobs,
	   qInfo[i].numJobs,
	   qInfo[i].numRUN,
	   qInfo[i].numPEND,
	   qInfo[i].numSSUSP,
	   qInfo[i].numUSUSP
	   );
  }
  return(0);
  
}
