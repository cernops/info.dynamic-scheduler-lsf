/*
   (C) CERN, Ulrich Schwickerath <ulrich.schwickerath@cern.ch>
   File:      $Id: statistics.c,v 1.1 2011/03/24 14:49:16 uschwick Exp $

   Purpose: Definitions for RMS calculations

   ChangeLog:
              $Log: statistics.c,v $
              Revision 1.1  2011/03/24 14:49:16  uschwick
              add btools toolsuite

              Revision 1.10  2008/01/21 18:42:53  uschwick
              bugfixes in accounting tool

              Revision 1.9  2007/08/22 19:53:57  dmicol
              Bug fixes.

              Revision 1.8  2007/08/22 10:12:58  dmicol
              Added support for the --user all option.

              Revision 1.7  2007/08/20 16:00:48  dmicol
              Added filtering by user.

              Revision 1.6  2007/08/20 11:34:48  dmicol
              Added support for statistics by user.

              Revision 1.5  2007/02/05 15:46:28  uschwick
              added new field: target LSF host type

              Revision 1.4  2007/01/15 17:05:55  uschwick
              added two new special groups in bacctinfo: other and all

              Revision 1.3  2006/12/19 14:05:57  uschwick
              output additional information in bjobsinfo

              Revision 1.2  2006/11/22 10:31:22  uschwick
              changed algorithm and added day-by-day statistics

              Revision 1.1  2006/11/18 11:19:18  uschwick
              code cleanup


   
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "statistics.h"


static struct lst_user_sum * all_users_sum = NULL;
static struct lst_user_sum ** index_users_sum = NULL;
static int number_of_users = 0;


struct lst_user_sum * GetUser(int user_id){
  return index_users_sum[user_id];
}

int GetUserId(const char * user_name, const char * options){
  int index;
  struct lst_user_sum * user_sum;

  user_sum = all_users_sum;
  index = 0;
  while (user_sum != NULL){
    /* check if it is the user we are looking for */
    if (strcmp(user_sum->user_name, user_name) == 0 && strcmp(user_sum->options, options) == 0){
      return index;
    } else {
      user_sum = user_sum->next;
      index++;
    }
  }

  /* user not found */
  return -1;
}

int GetUserIdWithoutOptions(const char * user_name){
  int index;
  struct lst_user_sum * user_sum;

  user_sum = all_users_sum;
  index = 0;

  while (user_sum != NULL){
    /* check if it is the user we are looking for */
    if (strcmp(user_sum->user_name, user_name) == 0){
      return index;
    } else {
      user_sum = user_sum->next;
      index++;
    }
  }

  /* user not found */
  return -1;
}

int AddUser(const char * user_name, const char * options){
  int i;
  struct lst_user_sum * new_user;
  struct lst_user_sum ** old_index;

  old_index = NULL;

  /* create the entry for the new user */
  new_user = (struct lst_user_sum *) malloc(sizeof(struct lst_user_sum) + (strlen(user_name) + 1) * sizeof(char));
  new_user->user_name = (char *) malloc((strlen(user_name) + 1) * sizeof(char));
  new_user->options = (char *) malloc((strlen(options) + 1) * sizeof(char));
  strcpy(new_user->user_name, user_name);
  strcpy(new_user->options, options);
  new_user->next = NULL;

  if (all_users_sum == NULL){
    /* if the list was empty, set the new element as the head */
    all_users_sum = new_user;
    new_user->prev = NULL;
  } else {
    /* otherwise, add the new user to the tail of the list */
    struct lst_user_sum * ptr = all_users_sum;
    while (ptr->next != NULL){
      ptr = ptr->next;
    }
    ptr->next = new_user;
    new_user->prev = ptr;

    /* copy the elements of the old index */
    old_index = (struct lst_user_sum **) malloc(number_of_users * sizeof (struct lst_user_sum *));
    for (i = 0; i < number_of_users; i++){
      old_index[i] = index_users_sum[i];
    }
  }

  /* create the new index */
  index_users_sum = (struct lst_user_sum **) malloc((number_of_users + 1) * sizeof (struct lst_user_sum *));

  /* put back the elements from the old one */
  if (number_of_users > 0){
    for (i = 0; i < number_of_users; i++){
      index_users_sum[i] = old_index[i];
    }
    free(old_index);
  }

  /* add the pointer to the new user */
  index_users_sum[number_of_users] = new_user;

  /* return the index of the new user */
  return number_of_users++;
}

void DeleteUser(int user_id){
  int i, j;
  struct lst_user_sum * user_sum;
  struct lst_user_sum ** old_index_users_sum;

  user_sum = index_users_sum[user_id];
  old_index_users_sum = NULL;

  /* unlink the node to be removed from the list */
  if (user_sum->prev != NULL) {
    user_sum->prev->next = user_sum->next;
  }
  if (user_sum->next != NULL) {
    user_sum->next->prev = user_sum->prev;
  }

  /* if it was the first element, reset the head of the list */
  if (all_users_sum == user_sum) {
    all_users_sum = user_sum->next;
  }

  /* delete the element */
  free(user_sum->user_name);
  free(user_sum->options);
  free(user_sum);

  /* reconstruct the index */
  if (number_of_users - 1 > 0) {
    old_index_users_sum = (struct lst_user_sum **) malloc(sizeof(struct lst_user_sum *) * (number_of_users - 1));
    for (i = 0, j = 0; i < number_of_users; i++) {
      if (i != user_id) {
        old_index_users_sum[j++] = index_users_sum[i];
      }
    }
  }

  free(index_users_sum);
  index_users_sum = NULL;

  number_of_users--;

  if (number_of_users > 0) {
    index_users_sum = (struct lst_user_sum **) malloc(sizeof(struct lst_user_sum *) * number_of_users);
    for (i = 0; i < number_of_users; i++) {
      index_users_sum[i] = old_index_users_sum[i];
    }
  }

  free(old_index_users_sum);
  old_index_users_sum = NULL;
}

void DeleteUsers(){
  struct lst_user_sum * user_sum, * next_user_sum;

  user_sum = all_users_sum;

  /* loop through all users and delete them */
  while (user_sum != NULL){
    next_user_sum = user_sum->next;
    DeleteUser(0);
    user_sum = next_user_sum;
  }
}

void InitRunningSums(struct lst_user_sum * user_sum, int entries){
  int i;
  for (i=0;i<entries;i++){
    user_sum->summed[i] = 0;
    user_sum->summed_square[i] = 0;
  }
  user_sum->nevents = 0;
}

void ResetRunningSums(struct lst_user_sum * user_sum, int entries){
  int i;
  for (i=0;i<entries;i++){
    user_sum->summed[i] = 0;
    user_sum->summed_square[i] = 0;
  }
  user_sum->nevents = 0;
}

void RunningSumsAddValues(struct lst_user_sum * user_sum, double value[],int entries){
  int i;
  //printf("DEBUG: adding values for user %s\n",user_sum->user_name);
  for (i=0;i<entries;i++){
    user_sum->summed[i] += value[i];
    user_sum->summed_square[i] += value[i] * value[i];
  }
  user_sum->nevents++;
}

int RunningSumsGetStatistics(struct lst_user_sum * user_sum, double * total, double * mean, double * standarddev, int entries){
  int i;
  double N;
  N = (user_sum->nevents>0) ? (double)user_sum->nevents:0.0;
  for (i=0;i<entries;i++){
    total[i] = user_sum->summed[i];
    if (N > 0.0) {
      mean[i] = user_sum->summed[i]/N;
      if (N > 1.0){
        standarddev[i]=sqrt((N*user_sum->summed_square[i]-user_sum->summed[i]*user_sum->summed[i])/(N*(N-1)));
      } else {
	standarddev[i]=mean[i];
      }
    } else {
      standarddev[i] = mean[i] = 0.0;
    }
  }
  return user_sum->nevents;
}

