/*
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch>
   File:      $Id: statistics.h,v 1.1 2011/03/24 14:49:16 uschwick Exp $

   Purpose: Definitions for standard deviation calculations

   ChangeLog:
              $Log: statistics.h,v $
              Revision 1.1  2011/03/24 14:49:16  uschwick
              add btools toolsuite

              Revision 1.8  2008/01/15 09:58:18  uschwick
              fixed memory corruption problem

              Revision 1.7  2007/08/22 19:53:57  dmicol
              Bug fixes.

              Revision 1.6  2007/08/22 10:12:58  dmicol
              Added support for the --user all option.

              Revision 1.5  2007/08/20 16:00:48  dmicol
              Added filtering by user.

              Revision 1.4  2007/08/20 11:34:48  dmicol
              Added support for statistics by user.

              Revision 1.3  2007/01/15 17:05:44  uschwick
              added two new special groups in bacctinfo: other and all

              Revision 1.2  2006/11/22 10:31:22  uschwick
              changed algorithm and added day-by-day statistics

              Revision 1.1  2006/11/18 11:19:17  uschwick
              code cleanup

   
*/

#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include <stdlib.h>
/* 
   group index : 4 bits
   Bit 0, 1 :    00 undef,other
                 01 running in GRID queue
                 10 submisson from other host
                 11 submission from any host
   Bit 2    :    0  this period only
                 1  accumulated/total
*/
#define MAXGROUPS 500 //64
#define MAXNUM 40

/* define a list that holds all values for which mean value and standard deviation are to be calculated */
struct value_list {
  struct value_list * next;
  double value[MAXNUM];
};

/* define a double-linked list node that contains user statistics */
struct lst_user_sum {
  char * user_name;
  char * options;
  double summed[MAXNUM];
  double summed_square[MAXNUM];
  int nevents;
  struct lst_user_sum * prev;
  struct lst_user_sum * next;
};

extern struct value_list * value_list_root_all;
extern struct value_list * value_list_root_grid;
extern struct value_list * value_list_root_local;

/* prototype definitions */

struct lst_user_sum * GetUser(int user_id);
int GetUserId(const char * user_name, const char * options);
int GetUserIdWithoutOptions(const char * user_name);
int AddUser(const char * user_name, const char * options);
void DeleteUser(int user_id);
void DeleteUsers();
void InitRunningSums(struct lst_user_sum * user_sum, int entries);
void ResetRunningSums(struct lst_user_sum * user_sum, int entries);
void RunningSumsAddValues(struct lst_user_sum * user_sum, double value[],int entries);
int RunningSumsGetStatistics(struct lst_user_sum * user_sum, double * total, double * mean, double * standarddev, int entries);

#endif
