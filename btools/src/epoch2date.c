/*
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch>
   File:      $Id: epoch2date.c,v 1.1 2011/03/24 14:49:16 uschwick Exp $
   ChangeLog:
              $Log: epoch2date.c,v $
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
time_t epoch;

void usage(){
  printf("Usage epoch2date --epoch <epoch> [--help] [--quiet] \n");
  exit(0);
}

void readOptions(int argc, char* argv[]){
  int c;
  unsigned int input;
  
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"help",   no_argument, 0, 0},
      {"quiet",  no_argument, 0, 0},
      {"epoch",  required_argument, 0, 0},
      {0, 0, 0, 0}
    };
    
    c = getopt_long (argc, argv, "e:hq",
		     long_options, &option_index);
    if (c == -1)
      break;
    
    switch(c) {
    case '?':
      usage();
      break;
      
    case 0:
      if (optarg){
	if (0 == strncmp(long_options[option_index].name,"epoch",5)){
	  if (0==sscanf(optarg,"%d",&input)){
	    usage();
	  }
	  epoch = (time_t) input;
	  break;
	} 
      } else {
	if (0 == strncmp(long_options[option_index].name,"quiet",5)){
	  quiet = 1;
	  break;
	} 
	if (0 == strncmp(long_options[option_index].name,"help",5)){
	  usage();
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

  epoch = 0;
  readOptions(argc,argv);

  if (0 == epoch){
    fprintf(stderr,"[ERROR] missing required argument epoch \n");
    usage();
  }

  if (quiet){
    printf("%s\n",ctime(&epoch));
  } else {
    printf("%s %d\n",ctime(&epoch),(unsigned int) epoch);
  }

  return(0);
}
