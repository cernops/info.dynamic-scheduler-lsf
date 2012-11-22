/* 
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch> 
   File:      $Id: bhostsinfo.c,v 1.1 2011/03/24 14:49:16 uschwick Exp $
   ChangeLog: 
              $Log: bhostsinfo.c,v $
              Revision 1.1  2011/03/24 14:49:16  uschwick
              add btools toolsuite

              Revision 1.7  2006/12/08 17:05:35  uschwick
              added protection against crazy real_Load values in bhostsinfo

              Revision 1.6  2006/12/05 12:38:26  uschwick
              removed debugging statement from bhostinfo

              Revision 1.5  2006/12/04 14:14:33  uschwick
              bugfix for hosts_OK

              Revision 1.4  2006/11/27 14:14:14  uschwick
              extended the output of bhostsinfo

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
#include <string.h>
#include <lsf/lsf.h>
#include <lsf/lsbatch.h>
#include <getopt.h>

char resource[4096];

void PrintStatus(int status){
  /*
  void PrintBit(int status){
    if (status){
      printf(" 1");
    } else {
      printf(" 0");
    }
  }
  PrintBit((int)(status == HOST_STAT_OK));
  PrintBit(status & HOST_STAT_FULL);
  PrintBit(status & HOST_STAT_BUSY);
  PrintBit(status & HOST_STAT_DISABLED);
  PrintBit(status & HOST_STAT_LOCKED);
  PrintBit(status & HOST_STAT_WIND);
  PrintBit(status & HOST_STAT_UNREACH);
  PrintBit(status & HOST_STAT_UNAVAIL);
  PrintBit(status & HOST_STAT_UNLICENSED);
  PrintBit(status & HOST_STAT_NO_LIM);
  */
  if (status == HOST_STAT_OK){
    printf("ok");
  } else if (status & HOST_STAT_FULL){
    printf("closed_Full");
  } else if (status & HOST_STAT_BUSY){
    printf("closed_Busy");
  } else if (status & HOST_STAT_DISABLED){
    printf("closed_Adm");
  } else if (status & HOST_STAT_LOCKED){
    printf("closed_Adm");
  } else if (status & HOST_STAT_WIND){
    printf("closed_Adm");
  } else if (status & HOST_STAT_UNREACH){
    printf("unreach");
  } else if (status & HOST_STAT_UNAVAIL){
    printf("unavail");
  } else if (status & HOST_STAT_UNLICENSED){
    printf("unlic");
  } else if (status & HOST_STAT_NO_LIM){
    printf("nolim");
  }
}

void usage(){
  printf("Usage bhostsinfo  [-h|--help] [-R resource] \n");
  exit(0);
}

void readOptions(int argc, char* argv[]){
  int c;
  extern char *optarg;

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"help",   no_argument, 0, 0},
      {"resource",  required_argument, 0, 0},
      {0, 0, 0, 0}
    };
    
    c = getopt_long (argc, argv, "R:h",
		     long_options, &option_index);
    if (c == -1)
      break;
    
    switch(c) {
    case '?':
      usage();
      break;
    case 'h':
      usage();
      break;
    case 'R':
      if (optarg){
	if (0 == strncpy(resource,optarg,4096)){
	  usage();
	}
      }
      break;
    case 0:
      if (0 == strncmp(long_options[option_index].name,"help",5)){
	usage();
      } 
      break;

    default:
      fprintf(stderr,"[ERROR] Unknown option\n");
      usage();
    }
  }
}

int main(int argc, char* argv[]){
  struct hostInfoEnt *hInfo1;
  struct hostInfo    *hInfo2;
  
  char * hostname = NULL;
  int numHosts1 = 0;
  int numHosts2 = 0;
  int options   = 0;
 
  int i,j,k;
  double real_load = 0.0;
  double load = 0.0;
  
  char* Resource = NULL;

  resource[0]=0;
  readOptions(argc,argv);
  if (0<strlen(resource)) {
    Resource = resource;
  }

  if (lsb_init(argv[0]) < 0){
    lsb_perror("bhostsinfo: lsb_init() failed");
    exit (-1);
  }
  
  /* query LSF */
  hInfo1 = lsb_hostinfo(&hostname,&numHosts1);
  if (hInfo1 == NULL){
    lsb_perror("bhostsinfo: lsb_hostinfo() failed");
    exit(-1);
  }
  
  /* query LSF */
  hInfo2 = ls_gethostinfo(resource,&numHosts2,NULL,0,options);
  if (hInfo2 == NULL){
    printf("Not enough hosts\n");
    exit(0);
  }
  if (0 == numHosts1 || 0 == numHosts2){
    printf("Not enough hosts");
    exit (0);
  } else { 
    for (i=0;i<numHosts1;i++){
      for (j=0;j<numHosts2;j++){
	if (0 == strncmp(hInfo1[i].host,hInfo2[j].hostName,MAXHOSTNAMELEN)){
	  /* in some cases real_load is undefined. Add a protection here */
	  if ((load = *(hInfo1[i].load))>5000){
	    load = 0.0;
	  };
	  if ((real_load = *(hInfo1[i].realLoad))>5000){
	    real_load = load;
	  };
	  printf("%s %s %f %f %f %d %d %d %d %d %d ",
		 hInfo1[i].host,
		 hInfo2[j].hostType,
		 hInfo1[i].cpuFactor,
		 load,
		 real_load,
		 hInfo1[i].maxJobs,
		 hInfo1[i].numJobs,
		 hInfo1[i].numRUN,
		 hInfo1[i].numSSUSP,
		 hInfo1[i].numUSUSP,
		 hInfo1[i].attr
		 );
	  for (k=0;k<hInfo2[j].nRes;k++){
	    printf("%s",hInfo2[j].resources[k]);
	    if (k<hInfo2[j].nRes-1){
	      printf(",");
	    } else {
	      printf(" "); 
	    }
	  }
	  PrintStatus(hInfo1[i].hStatus);
	  printf("\n");
	  break;
	}
      }
    }
    return(0);
  }
}
