/*********************************************************************************
 * Copyright (c) 2018                                                            *
 * Ali Omar abdelazim Mohammed <ali.mohammed@unibas.ch>                          *
 * University of Basel, Switzerland                                              *
 * All rights reserved.                                                          *
 *                                                                               *
 * This program is free software; you can redistribute it and/or modify it       *
 * under the terms of the license (GNU LGPL) which comes with this package.      *
 *********************************************************************************/


/* simulate the execution of PSIA on miniHPC cluster */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "simgrid/simdag.h"
#include "simgrid/msg.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include "add_host_attributes.c"
#include "dls.c"
#include "dls_support.c"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");



int main(int argc, char **argv)
{
  //for reading the tasks status after they finish
  unsigned int cursor;
  SD_task_t task;
  int total_nhosts = 0;
  int numTasks = 0;
  int scheduledTasks = 0;
  sg_host_t *hosts = NULL;
  sg_host_t temp_host;
  int start_task = 0;
  double start_time = 0;
  double period = 50;
  double endtime;
  int finished_tasks = 0;


 typedef struct CHUNK {
    int start;
    int size;
   } CHUNK;


  SD_task_t calculate_chunk;



  sg_host_t *executedHosts;
  xbt_dynar_t changed_tasks;
  char taskName[64];
  int chunk_size = 1;
  int PSIZE = 4;
  int scheduling_step = 0;
  int METHOD = 1;
  char *task_times_file;
  int avail_cores = 0;
  int num_idle_cores = 0;
  int  idle_core_ID = -1;
  int cores_per_host = 1;
  int i,j, k;
  int image_width = 5;
  int numHosts = 1;
  double sigma = 0.0105757212848; //median for 352 processes 400K
  double h_overhead = 0.0060245; //median for 352 processes 400K
  double h_flops = 0;



  int requestWhen = 50;
  int breakAfter = 100;


  /* initialization of SD */
  SD_init(&argc, argv);


  /* Check our arguments */
  xbt_assert(argc > 6, "Usage: %s platform_file Number_of_hosts Cores_per_host Problem_size Scheduling_method FLOPs_filename scheduling_overhead iterations_std \n Scheduling Methods \n0: Static Chunking \n1: Self Scheduling \n2: Fixed size chunking\n3: Guided Self Scheduling\n4: Factoring \n"
             "\n""\tExample: %s xeon_platform.xml 9 1 200 1 flops_summary.csv 1 0.1 start_task  start_time period", argv[0], argv[0]);


  /* creation of the environment */
  SD_create_environment(argv[1]);
  numHosts = atoi(argv[2]);
  cores_per_host = atoi(argv[3]);
  PSIZE  = atoi(argv[4]);
  METHOD = atoi(argv[5]);
  task_times_file = argv[6];
  if (argc > 7)
  {
    h_overhead = atof(argv[7]);
  }
  if (argc > 8)
  {
    sigma = atof(argv[8]);
  }
  if (argc > 9)
  {
   start_task = atoi(argv[9]); //inloop start task
  }

  if (argc > 10)
  {
   start_time = atof(argv[10]); //inloop start time
  }

  if (argc > 11)
  {
   period = atof(argv[11]); // simulation period
  }

  endtime = period + start_time;

  int remainingTasks = PSIZE - start_task;

  if((METHOD > 12) || (METHOD < 0))
   {
       printf("ERROR: Unsupported DLS technique, fall back to STATIC \n");
       METHOD = 0;
   }

  avail_cores = numHosts* cores_per_host;
// assined tasks, to keep track of the assigned tasks, only schedule breakafter or requestwhen values at a time
    CHUNK *assigned_chunks = malloc(avail_cores*sizeof(CHUNK));
    for (i = 0; i < avail_cores; i++ )
    {
        assigned_chunks[i].start = 0;
        assigned_chunks[i].size = 0;
    }

  /*  Allocating the host attribute */
  total_nhosts = sg_host_count();
  hosts = sg_host_list();

  //sort hosts
  for (i = 0; i < total_nhosts; i++) {
    for (j = i+1; j < total_nhosts; j++) {
      if (find_number(SD_workstation_get_name(hosts[i])) > find_number(SD_workstation_get_name(hosts[j]))) {
        temp_host = hosts[i];
        hosts[i] = hosts[j];
        hosts[j] =  temp_host;

      }
    }
  }
int chunk_counter[avail_cores];
for(i= 0; i< avail_cores;i++)
{
	chunk_counter[i] = 0;
}

  for (i = 0; i < numHosts; i++) {
    sg_host_allocate_attribute_w_cores(hosts[i], cores_per_host);
  }

  /* initilize the adaptive techniques */
  init_adaptive_weights(avail_cores);
  init_worker_bookkeep(avail_cores);
  calculate_initial_weights(hosts, avail_cores);

  numTasks = PSIZE - start_task;
  // create tasks
   taskArray = create_PSIA_tasks(PSIZE,numHosts,METHOD,task_times_file, start_task);
 xbt_dynar_t changed_tasks_dynar = xbt_dynar_new(sizeof(SD_task_t), NULL);

   //printf("time before loop is %lf \n",SD_get_clock());
// time offset
  // create a task to advance the simulation time to the start time
  double core_speed = sg_host_core_speed(hosts[1]);
  SD_task_t time_offset = SD_task_create_comp_seq(taskName, NULL, start_time*core_speed);
  SD_schedule_task_on_host_onCore(time_offset, hosts[1], 0);
  SD_simulate_with_update(start_time, changed_tasks_dynar);

  //printf("time before loop is %lf \n",SD_get_clock());
  //-------------------------------------------begin main work--------------------------------------------------------------
  double tpar1 = 0;
// measure time at the begining of the parallel computation
  //SD_task_t tpar1_task = sg_host_get_last_scheduled_task_on_core(hosts[0], 0);
   tpar1 = SD_get_clock();
   int assignedTasks= 0;
  //start scheduling and parallel computation
while ((!xbt_dynar_is_empty(changed_tasks_dynar))|| (scheduledTasks < numTasks)||(assignedTasks< numTasks))
  {
/*
    //print information about changed tasks
    xbt_dynar_foreach(changed_tasks, cursor, task)
    {
      if (SD_task_get_state(task) == SD_DONE) //if the task is finished
      {
       // XBT_INFO("Task '%s' start time: %f, finish time: %f", SD_task_get_name(task), SD_task_get_start_time(task),SD_task_get_finish_time(task));
        //XBT_INFO("Task amount is %lf",SD_task_get_amount(task));
        //numHosts = SD_task_get_workstation_count(task);
        //executedHosts = SD_task_get_workstation_list(task);
        //for (int i = 0; i < numHosts; i++) {
        //XBT_INFO("Task '%s' executed on %s ",SD_task_get_name(task),SD_workstation_get_name(executedHosts[i]) );
        }
      }

    }
  */

    /*   need to bookkeep the the previous chunk size of each PE and the time spent in executing this chunk */
      //main scheduling loop
      //
/*
      for(i = 0; i< numHosts; i++)
      {
         printf("%d: assigned_size: %d, start = %d \n",i, assigned_chunks[i].size, assigned_chunks[i].start );
      }
*/
      for (i = 0; i < numHosts; i++)
      {
         num_idle_cores = sg_host_num_idle_cores(hosts[i]);

         for(k = 0; (assignedTasks < numTasks) && (k < num_idle_cores); k++)
       {
           //get the ID of the idle core on current host
           idle_core_ID = sg_host_idle_core_id(hosts[i]);
	   int PE_id = i*cores_per_host+idle_core_ID;
           //printf("%d: is free \n",PE_id);
           // take from the already assigned chunk
           if(assigned_chunks[PE_id].size > 0)
           {
                //printf("%d: take up from the assigned chunk \n",PE_id);
                int limit = (PE_id == 0) ? breakAfter:assigned_chunks[PE_id].size;
               if(PE_id != 0) // a worker not master -- request a new chunk
               {
                  send_work_request(METHOD, scheduling_step, avail_cores, PE_id, cores_per_host, hosts);
                  calculate_chunk = create_scheduling_overhead_task(METHOD,scheduling_step,avail_cores, PE_id);
                  SD_schedule_task_on_host_onCore(calculate_chunk, hosts[0], 0); // scheduling overhead on master
               }
              // schedule the rest of the tasks on core
	      for (j = 0; (j < limit)&&(assigned_chunks[PE_id].size > 0); j++) {

              SD_schedule_task_on_host_onCore(taskArray[assigned_chunks[PE_id].start], hosts[PE_id/cores_per_host], idle_core_ID);
              assigned_chunks[PE_id].start++;
              assigned_chunks[PE_id].size--;
              assignedTasks++;
              //XBT_INFO("Host %d available power is %f \n", i, SD_workstation_get_available_power(hosts[i]));
           }
          // printf("%d: assigned_size: %d, start = %d \n",i, assigned_chunks[i].size, assigned_chunks[i].start );
           //request a new chunk
           }
           else
           {
	   // update the times for AWF without overhead
	   //printf("%d taking a new chunk \n", PE_id);
	   update_chunk_time(PE_id);

            // update the times for AWF

            update_chunk_time_w_overheads(PE_id);

            //calculate chunk size
            remainingTasks = numTasks - scheduledTasks;
            chunk_size = calculate_chunk_size(numTasks,avail_cores,scheduling_step, remainingTasks, METHOD,h_overhead,sigma, PE_id);
            //printf("chunksize, %d \n", chunk_size);
            if(chunk_size <= 0) // work is finished
		continue;
            // update status and times for the AWF techniues
            previous_chunk_size[PE_id] = chunk_size;
            chunk_start_task_id[PE_id] = scheduledTasks;
            last_request_time[PE_id] = SD_get_clock();
	    previous_step[PE_id]++;
            chunk_counter[PE_id]++;
            //put the scheduled tasks in the assigned chunks
            assigned_chunks[PE_id].start = scheduledTasks;
            assigned_chunks[PE_id].size = chunk_size;
            //update scheduled tasks
            scheduledTasks += chunk_size;
            //printf("chunk size, %d \n", chunk_size);
            // take from assigned tasks only until breakAfter or requestWhen
            int limit = (PE_id == 0) ? breakAfter:(chunk_size - requestWhen);
            for (j = 0; (j < limit)&&(assigned_chunks[PE_id].size > 0); j++)
            {
              SD_schedule_task_on_host_onCore(taskArray[assigned_chunks[PE_id].start], hosts[PE_id/cores_per_host], idle_core_ID);
              assigned_chunks[PE_id].start++;
              assigned_chunks[PE_id].size--;
              assignedTasks++;
             }

            // update status and times for the AWF techniues
            chunk_finish_task_id[PE_id] = scheduledTasks - 1;

            //increase scheduling_step by 1
            scheduling_step++;
            //printf("scheduling step %d\n",scheduling_step);
            } // end else
            }//end for
            //printf("scheduled tasks: %d\n", scheduledTasks);
        } //end for

	xbt_dynar_reset(changed_tasks_dynar);
	SD_simulate_with_update(-1.0, changed_tasks_dynar);

        // if current time is greater than or equal end time, end the simulation
        if(SD_get_clock() >= endtime)
        {
    //      printf("time is %lf, I am out \n", SD_get_clock());
           break; }


      }  //end while

  //    printf("Experiment_parameters\n");

   //   printf("Problem %d\n",PSIZE);
   //   printf("Workers %d\n",avail_cores);
   //   printf("Method %d\n", METHOD);
   //   printf("-----------------------\n");

      //double tpar1 = 0;

      double tpar2 = SD_get_clock();
      //printf("Tpar is, %lf\n", tpar2 - tpar1);

       for( i = 0; i < numTasks; i++ )
        {
           if (SD_task_get_state(taskArray[i]) == SD_DONE) //if the task is finished
           {
             finished_tasks++;
           }
        }

       printf("Method, %d,  Parallel time, %lf, scheduled tasks, %d, finished tasks, %d\n", METHOD, tpar2 - tpar1, scheduledTasks,finished_tasks );

      for(i = 0; i<numTasks; i++){
  	  SD_task_destroy(taskArray[i]);
      }

      for (i = 0; i < numHosts; i++) {
   	   sg_host_free_attribute(hosts[i]);
      }

  return 0;
}
