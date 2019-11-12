/*********************************************************************************
 * Copyright (c) 2017                                                           *
 * Ali Omar abdelazim Mohammed <ali.mohammed@unibas.ch>                          *
 * University of Basel, Switzerland                                              *
 * All rights reserved.                                                          *
 *                                                                               *
 * This program is free software; you can redistribute it and/or modify it       *
 * under the terms of the license (GNU LGPL) which comes with this package.      *
 *********************************************************************************/
#include "simgrid/msg.h"

// adding last scheduled task property to hosts
typedef struct _HostAttribute *HostAttribute;

struct _HostAttribute {
    int core_count;
    SD_task_t *last_scheduled_task;
};

double sg_host_core_speed(sg_host_t host)
{
	return  MSG_host_get_speed(host);	
}


int sg_host_core_count(sg_host_t host) {
  //return MSG_host_get_core_number(host);
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  return attr->core_count;
  }

static void sg_host_allocate_attribute(sg_host_t host)
{
    void *data;
    int nCores =  sg_host_core_count(host);
    data = calloc(1, sizeof(struct _HostAttribute));
    HostAttribute attr = (HostAttribute) data;
    attr-> core_count = MSG_host_get_core_number(host);
    attr->last_scheduled_task = calloc(nCores, sizeof(SD_task_t));
    sg_host_user_set(host, data);
}

static void sg_host_allocate_attribute_w_cores(sg_host_t host, int numCores)
{
    void *data;
    data = calloc(1, sizeof(struct _HostAttribute));
    HostAttribute attr = (HostAttribute) data;
    attr-> core_count = numCores;
    attr->last_scheduled_task = calloc(numCores, sizeof(SD_task_t));
    sg_host_user_set(host, data);
}


static void sg_host_free_attribute(sg_host_t host)
{
    HostAttribute attr = (HostAttribute) sg_host_user(host);
    free(attr->last_scheduled_task);
    free(sg_host_user(host));
    sg_host_user_set(host, NULL);
}

static SD_task_t sg_host_get_last_scheduled_task_on_core(sg_host_t host, int nCore){
    HostAttribute attr = (HostAttribute) sg_host_user(host);
    return attr->last_scheduled_task[nCore];
}

static void sg_host_set_last_scheduled_task_on_core(sg_host_t host, SD_task_t task, int nCore){
    HostAttribute attr = (HostAttribute) sg_host_user(host);
    attr->last_scheduled_task[nCore]=task;
    sg_host_user_set(host, attr);
}

static bool sg_host_is_core_idle(sg_host_t host, int nCore)
{
  if ((sg_host_get_last_scheduled_task_on_core(host,nCore) == NULL)||(SD_task_get_state(sg_host_get_last_scheduled_task_on_core(host,nCore)) == SD_DONE)) {
    return true;
  }
  else
  {
    return false;
  }
}
//get the idle core ID
static int sg_host_idle_core_id(sg_host_t host)
{
  int nCore = sg_host_core_count(host);
  int i;
  for (i = 0; i < nCore; i++) {
    if (sg_host_is_core_idle(host,i)) {
        return i;
    }
  }
  return -1;
}
// get the number of idle cores in a host
static int sg_host_num_idle_cores(sg_host_t host)
{
  int nCore = sg_host_core_count(host);
  int count = 0;
  int i;
  for (i = 0; i < nCore; i++) {
    if (sg_host_is_core_idle(host,i)) {
        count++;
    }
  }
  return count;
}


static bool sg_host_is_idle(sg_host_t host)
{
  bool state = true;
  int nCore = sg_host_core_count(host);
  int i;
  for (i = 0; i < nCore; i++) {
    state = state && sg_host_is_core_idle(host,i);
  }
  return state;
}

static int sg_host_next_idle_core_id(sg_host_t host, int start)
{
  int nCore = sg_host_core_count(host);
  int i;
  for (i = start+1; i < nCore; i++) {
    if (sg_host_is_core_idle(host,i)) {
        return i;
    }
  }
  return -1;
}

bool sg_host_is_all_idle(const sg_host_t *hosts, int num_hosts)
{
  bool is_idle = true;
  int i;

  for (i = 0; i < num_hosts; i++) {
    is_idle = is_idle && sg_host_is_idle(hosts[i]);
  }
  return is_idle;
}
