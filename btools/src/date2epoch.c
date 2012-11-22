/*
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch>
   File:      $Id: date2epoch.c,v 1.1 2011/03/24 14:49:16 uschwick Exp $
   ChangeLog:
              $Log: date2epoch.c,v $
              Revision 1.1  2011/03/24 14:49:16  uschwick
              add btools toolsuite

              Revision 1.4  2008/07/28 12:05:14  uschwick
              added quiet option + implemented security improvements in epoch tools

              Revision 1.3  2006/11/22 13:00:07  uschwick
              added documentation

              Revision 1.2  2006/11/18 11:24:44  uschwick
              added bjobsinfo man page to CVS

              Revision 1.1  2006/11/17 22:10:26  uschwick
              version 0.0.3 snapshot


   Purpose:  read a date from stdin and return the epoch

 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

struct tm Time;

int quiet = 0;

void usage(){
  printf("Usage: date2epoch [--quiet ] [--sec <seconds>] [--min <minutes>] [--hour <hours>] [--day <day>] [--month <month>] [--year <year>][--st [-1|0|1]]\n");
  exit(0);
}

void readOptions(int argc, char* argv[]){
  int c;
  int year;
  int month;
  int day;

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"sec",    1, 0, 0},
      {"min",    1, 0, 0},
      {"hour",   1, 0, 0},
      {"month",  1, 0, 0},      
      {"day",    1, 0, 0},      
      {"year",   1, 0, 0},
      {"st",     1, 0, 0},
      {"help",   no_argument, 0, 0},
      {"quiet",  no_argument, 0, 0},
      {0, 0, 0, 0}
    };
    
    c = getopt_long (argc, argv, "s:m:hr:d:mo:y:hq",
		     long_options, &option_index);
    if (c == -1)
      break;
    
    switch(c) {
    case '?':
      usage();
      break;
      
    case 0:
      if (optarg){
	if (0 == strncmp(long_options[option_index].name,"st",2)){
	  if (0==sscanf(optarg,"%d",&Time.tm_isdst)){
	    usage();
	  }
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"sec",3)){
	  if (0==sscanf(optarg,"%d",&Time.tm_sec)){
	    usage();
	  }
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"min",3)){
	  if (0==sscanf(optarg,"%d",&Time.tm_min)){
	    usage();
	  }
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"hour",4)){
	  if (0==sscanf(optarg,"%d",&Time.tm_hour)){
	    usage();
	  }
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"day",3)){
	  if (0==sscanf(optarg,"%d",&day)){
	    usage();
	  }
	  Time.tm_mday = day;
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"month",5)){
	  if (0 == sscanf(optarg,"%d",&month)){
	    usage();
	  }
	  Time.tm_mon = month - 1;
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"year",4)){
	  if (0==sscanf(optarg,"%d",&year)){
	    usage();
	  } 
	  Time.tm_year = year - 1900;
	  break;
	} 
      } else {
	if (0 == strncmp(long_options[option_index].name,"help",4)){
	  usage();
	  break;
	} 
	
	if (0 == strcmp(long_options[option_index].name,"quiet")){
	  quiet = 1;
	  break;
	} 
      }
    default:
      fprintf(stderr,"[ERROR] Unknown option %c\n",c);
      usage();
    }
  }
  return;
}

int main(int argc, char *argv[]){
  time_t epoch;
  struct tm * tm;

  tm = &Time;

  Time.tm_sec= 0;
  Time.tm_min= 0;
  Time.tm_hour= 0;
  Time.tm_mday = 0;
  Time.tm_mon = 0;
  Time.tm_year= 70;
  Time.tm_wday = 0;
  Time.tm_yday = 0;
  Time.tm_isdst = 0;

  readOptions(argc,argv);

  epoch = mktime(tm);
  if (quiet){
    printf("%d\n",(unsigned int) epoch);
  } else {
    printf("%s %d\n",ctime(&epoch),(unsigned int) epoch);
  }
  return(0);
}
