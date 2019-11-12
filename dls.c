/*********************************************************************************
 * Copyright (c) 2018                                                            *
 * Ali Omar abdelazim Mohammed <ali.mohammed@unibas.ch>                          *
 * University of Basel, Switzerland                                              *
 * All rights reserved.                                                          *
 *                                                                               *
 * This program is free software; you can redistribute it and/or modify it       *
 * under the terms of the license (GNU LGPL) which comes with this package.      *
 *********************************************************************************/

 #include <ctype.h>
#include <string.h>
#define MIN_CHUNK_SIZE 1


double *adaptive_weight;
double *weighted_average_ratio;
int *previous_chunk_size;
int *acc_chunk_size;
double *previous_chunk_time;
double *acc_chunk_time;
double *previous_chunk_sq_time; // for the sigma of AF
double *acc_chunk_sq_time;
double *previous_chunk_time_w_overhead;
double *num_weighted_average_ratio;
double *dem_weighted_average_ratio;
int *previous_step;
int is_calculated_AWF = 0; // flag for AWF

double *mu;
double *sigma2;
double AF_D = -1.0;
double AF_T = -1.0;
int current_batch_chunk_size = 1;
int TSS_chunk,TSS_delta;

void calculate_initial_weights(sg_host_t *hosts, int num_PE)
{
  double total_sum = 0.0;
  int i;
  for (i = 0; i < num_PE; i++)
  {
    total_sum += sg_host_core_speed(hosts[i]);
    //printf("PE %d speed: %lf \n", i, sg_host_core_speed(hosts[i]));
    //getchar();
  }
  for (i = 0; i < num_PE; i++)
  {
    adaptive_weight[i] =  sg_host_core_speed(hosts[i])/total_sum * num_PE;
    //printf("weight_PE[%d] = %lf \n", i,adaptive_weight[i]);
    //getchar();
  }
    //getchar();
}


void init_adaptive_weights(int num_PE)
{
  int i = 0;
  // initializing the adaptive weights
  adaptive_weight = malloc(num_PE*sizeof(double));
  weighted_average_ratio = malloc(num_PE*sizeof(double));
  previous_chunk_size = malloc(num_PE*sizeof(int));
  acc_chunk_size = malloc(num_PE*sizeof(int));
  previous_chunk_time = malloc(num_PE*sizeof(double));
  acc_chunk_time = malloc(num_PE*sizeof(double));
  previous_chunk_sq_time = malloc(num_PE*sizeof(double));
  acc_chunk_sq_time = malloc(num_PE*sizeof(double));
  previous_chunk_time_w_overhead = malloc(num_PE*sizeof(double));
  num_weighted_average_ratio = malloc(num_PE*sizeof(double));
  dem_weighted_average_ratio = malloc(num_PE*sizeof(double));
  previous_step = malloc(num_PE*sizeof(int));
  mu = malloc(num_PE*sizeof(double));
  sigma2 = malloc(num_PE*sizeof(double));

  for (i = 0; i < num_PE; i++) {
    adaptive_weight[i] = 1;
    weighted_average_ratio[i] = 0;
    previous_chunk_size[i] = 0;
    acc_chunk_size[i] = 0;
    acc_chunk_time[i] = 0.0;
    acc_chunk_sq_time[i] = 0.0;
    previous_chunk_time[i] = 0;
    previous_chunk_sq_time[i] = 0.0;
    previous_chunk_time_w_overhead[i] = 0;
    num_weighted_average_ratio[i] = 0;
    dem_weighted_average_ratio[i] = 0;
    previous_step[i] = 0;
    mu[i] = 0.0;
    sigma2[i] = 0.0;
  }
}

void calculate_AF_mu_sigma(int PE_id, double p_chunk_sq_time, double p_chunk_time, int p_chunk_size)
{
  if (p_chunk_time <= 0.0)
  {
    //printf("worker did not finish yet, exiting \n");
    return ;
  }
  // accummulate time and chunk size
  acc_chunk_size[PE_id] += p_chunk_size;
  acc_chunk_time[PE_id] += p_chunk_time;
  acc_chunk_sq_time[PE_id] += p_chunk_sq_time;

  mu[PE_id] = acc_chunk_time[PE_id] / (double) acc_chunk_size[PE_id];
  //printf("mu[%d]: %lf, chunk time: %lf, size: %d \n",PE_id, mu[PE_id], p_chunk_time, p_chunk_size);
  //getchar();
  sigma2[PE_id] = (acc_chunk_sq_time[PE_id] - mu[PE_id]*mu[PE_id]*acc_chunk_size[PE_id]) / (double) (acc_chunk_size[PE_id] - 1.0);
  sigma2[PE_id] = MAX(sigma2[PE_id], 0.0);
  //sigma2[PE_id] = sigma2[PE_id] < 0.0 ? 0 : sigma2[PE_id];

  //printf("PE_id: %d, chunk_time: %lf, chunk_size: %d, mu: %lf, sigma2: %lf\n",PE_id, p_chunk_time, p_chunk_size,mu[PE_id], sigma2[PE_id]);
  //reset chunk time and size
  previous_chunk_time[PE_id] = 0;
  previous_chunk_time_w_overhead[PE_id] = 0;
  previous_chunk_size[PE_id] = 0;

}

void calculate_AF_D_T(int num_PE)
{
  int i;
  // check if all workers have updated their sigmas
  for(i = 0; i < num_PE; i++)
  {
   if (mu[i] <= 0.0)
   {
     //printf("Not all workers have updated their sigmas, ... exiting\n");
     return ;
   }
  }

  // calcuate AF_D and AF_T
  double sum = 0.0;
  AF_D = 0;
  for (i = 0; i < num_PE; i++)
  {
    AF_D += (double) sigma2[i] / mu[i];
    //printf("mu[%d]= %lf, AF_D= %lf \n", i, mu[i],AF_D);
    sum += (double) 1.0 / mu[i];
  }

  AF_T = (double) 1.0 / sum ;

  //printf(" AF_D: %lf, AF_T: %lf \n", AF_D, AF_T);
  //getchar();
  //reseting
  //updated_sigmas = 0; commented to allow the updates to happen on every new measurement received
}


void update_weighted_average_ratio(int PE_id, int step, double p_chunk_time, int p_chunk_size)
{
    // accumulate the weighted average in this time step
    //printf("step: %d, chunk_time: %lf, chunk_size: %d, PE_id: %d \n", step, p_chunk_time,p_chunk_size,PE_id);
    if (p_chunk_time <= 0.0)
    {
      //printf("worker did not finish yet, exiting \n");
      return ;
    }
    num_weighted_average_ratio[PE_id] += step*p_chunk_time;
    dem_weighted_average_ratio[PE_id] += step*p_chunk_size;
    weighted_average_ratio[PE_id] = ((double) num_weighted_average_ratio[PE_id]) / ((double) dem_weighted_average_ratio[PE_id]);
    //reset chunk time and size
    previous_chunk_time[PE_id] = 0;
    previous_chunk_time_w_overhead[PE_id] = 0;
    previous_chunk_size[PE_id] = 0;

}


double calculate_average_weighted_ratio(int num_PE)
{
  double total_ratio = 0;
  int i;
  for (i = 0; i < num_PE; i++) {
    total_ratio += weighted_average_ratio[i];
  }
  return total_ratio/num_PE;
}

void update_adaptive_weights(int num_PE)
{
  double *raw_weights = malloc(num_PE*sizeof(double));
  double total_raw_weight = 0;
  double average_weighted_ratio = calculate_average_weighted_ratio(num_PE);
  int i;
  // check that all ranks have updated their average weighted total_ratio
  //printf("updated weighted ratio counter: %d \n", updated_weighted_ratios);
  for(i = 0; i< num_PE; i++)
  {
    if (weighted_average_ratio[i] <= 0.0)
    {
      //printf("Not all workers have updated their ratios, cannot update weights ...exiting \n");
      return ;
    }
  }
  // calcuate raw weights and their sum
  for (i = 0; i < num_PE; i++) {
    raw_weights[i] = average_weighted_ratio / weighted_average_ratio[i];
    total_raw_weight += raw_weights[i];
  }

  //update adaptive weights
  for (i = 0; i < num_PE; i++)
  {
    adaptive_weight[i] = raw_weights[i] * num_PE / total_raw_weight;
    //printf("PE_id: %d , weight: %lf \n", i, adaptive_weight[i]);
  }
  //getchar();
  is_calculated_AWF++; // adaptive weights are calculated
}

void update_all_weighted_average_ratio(int num_PE)
{
  int i;
  for(i = 0; i < num_PE; i++)
  {
      update_weighted_average_ratio(i,previous_step[i], previous_chunk_time[i], previous_chunk_size[i]);
  }
}


 int calculate_chunk_size(int num_tasks, int num_PE, int step, int remaining, int DLS, double h_overhead, double sigma, int PE_id)
 {
   int j = step / num_PE;
   int chunk_size = 1;
   int i;
   double K;
   int tSize;

   //printf("Method: %d, step: %d, Num_PE: %d, Num_tasks: %d, remaining: %d \n", DLS, step, num_PE, num_tasks, remaining);
   switch(DLS)
   {
     case 0:
     //static chunking
     chunk_size = ceil((double) num_tasks/num_PE);
     break;
     case 1:
     //self scheduling
     chunk_size = 1;
     break;
     case 2:
     //fixed size chunk
      K=(sqrt(2)*num_tasks*h_overhead)/(sigma*num_PE*sqrt(log(num_PE)));
     K=pow(K, 2.0/3.0);
     chunk_size = (int) ceil(K);
     break;
     case 3:
     //GSS
     chunk_size = ceil((double) remaining/num_PE);
     break;
     case 4:
     // TSS
     if(step == 0 )
     {
        TSS_chunk = ceil((double) num_tasks / ((double) 2*num_PE));
        int n = ceil(2*num_tasks/(TSS_chunk+1)); //n=2N/f+l
        TSS_delta = (double) (TSS_chunk - 1)/(double) (n-1);
        chunk_size = TSS_chunk;
     }
     else
     {
        chunk_size = TSS_chunk - TSS_delta;
        TSS_chunk = chunk_size;
     }
     break;
     case 5:
     //factoring -- should be called only every batch
     if(step%num_PE == 0) // new batch
     {
       chunk_size = ceil((double) remaining/(2*num_PE));
       current_batch_chunk_size = chunk_size;
     }
     else
     {
       chunk_size = current_batch_chunk_size;
     }
        //chunk_size = ceil(pow(0.5,j+1)*num_tasks/(double) num_PE);
     break;
     case 6:
     // weighted factoring
     if(step%num_PE == 0) // new batch
     {
       chunk_size = ceil((double) remaining/(2*num_PE));
       current_batch_chunk_size = chunk_size;
       chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
     }
     else
     {
       chunk_size = current_batch_chunk_size;
       chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
     }
      // chunk_size = ceil(pow(0.5,j+1)*num_tasks/(double) num_PE);
       //chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
     break;
     case 7:
      // AWF-B
       update_weighted_average_ratio(PE_id,previous_step[PE_id], previous_chunk_time[PE_id], previous_chunk_size[PE_id]);
       if(step%num_PE == 0) // new batch
       {
       update_adaptive_weights(num_PE);
       if (is_calculated_AWF == 0) //first chunk 10% or AWF is not yet available
       {
          chunk_size = ceil(0.1*num_tasks/(double) num_PE);
          current_batch_chunk_size = chunk_size;
       }
       else
       {
         chunk_size = ceil((double) remaining/(2*num_PE));
         current_batch_chunk_size = chunk_size;
         chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
       }
      }
      else // use old chunk size
      {
        chunk_size = current_batch_chunk_size;
        if(is_calculated_AWF != 0)  // use AWF
        {
          chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
        }
      }

      //printf("step: %d, AWF_flag: %d, PE: %d, chunk: %d \n",step,is_calculated_AWF, PE_id, chunk_size);
      //getchar();

     break;
     case 8:
     // AWF-C
     update_weighted_average_ratio(PE_id,previous_step[PE_id], previous_chunk_time[PE_id], previous_chunk_size[PE_id]);
     update_adaptive_weights(num_PE);
     if (is_calculated_AWF == 0) // first chunk 10%, or AWF is not yet avaialable
      {
          chunk_size = ceil(0.1*num_tasks/(double) num_PE);
          //printf("first batch, chunksize: %d \n", chunk_size);
      }
      else
      {
       chunk_size = ceil((double) remaining/(2*num_PE));
       chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
      }
     break;
     case 9:
     // AWF-D
     update_weighted_average_ratio(PE_id,previous_step[PE_id], previous_chunk_time_w_overhead[PE_id], previous_chunk_size[PE_id]);
     if(step%num_PE == 0) // new batch
     {
       update_adaptive_weights(num_PE);
       if (is_calculated_AWF == 0) //first chunk 10% or AWF is not yet available
       {
          chunk_size = ceil(0.1*num_tasks/(double) num_PE);
	  current_batch_chunk_size = chunk_size;
       }
       else
       {
 	 chunk_size = ceil((double) remaining/(2*num_PE));
         current_batch_chunk_size = chunk_size;
         chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
       }
     }
     else // use old chunk size
     {
        chunk_size = current_batch_chunk_size;
        if(is_calculated_AWF != 0)  // use AWF
        {
          chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
        }
     }

    //printf("step: %d, AWF_flag: %d, PE: %d, chunk: %d \n",step,is_calculated_AWF, PE_id, chunk_size);
     // getchar();
     break;
     case 10:
     // AWF-E
     update_weighted_average_ratio(PE_id,previous_step[PE_id], previous_chunk_time_w_overhead[PE_id], previous_chunk_size[PE_id]);
     update_adaptive_weights(num_PE);
     if ( is_calculated_AWF == 0) // first chunk 10% or AWF is not yet available
      {
          chunk_size = ceil(0.1*num_tasks/(double) num_PE);
          //printf("first batch, chunksize: %d \n", chunk_size);
      }
      else
      {
       chunk_size = ceil((double) remaining/(2*num_PE));
       chunk_size = ceil(chunk_size*adaptive_weight[PE_id]);
     }
     break;
     case 11:
     // adaptive factoring AF
     //factoring in the first chunk
     calculate_AF_mu_sigma(PE_id, previous_chunk_sq_time[PE_id], previous_chunk_time[PE_id], previous_chunk_size[PE_id]);
     calculate_AF_D_T(num_PE);
     if ((AF_D < 0.0 ) || (AF_T < 0.0 )) // first chunk size ...should be 10%
      {
          chunk_size = ceil(0.1*num_tasks/(double) num_PE);
          //printf("first batch, chunksize: %d, PE: %d \n", chunk_size, PE_id);
      }
      else
      {
       chunk_size = (double) (AF_D + 2*AF_T*remaining - sqrt(AF_D*AF_D + 4*AF_D*AF_T*remaining)) / (2*mu[PE_id]);
       chunk_size = ceil(chunk_size);
      //printf(" AF_D: %lf, AF_T: %lf, chunk size: %d, mu: %lf, PE: %d\n", AF_D, AF_T, chunk_size, mu[PE_id], PE_id);

     }
     break;
     case 12:
     // modified FSC mFSC
     tSize = (num_tasks+num_PE-1)/num_PE;
     chunk_size = (0.55+tSize*log(2.0)/log( (1.0*tSize)));
     break;
     default:
     printf("Error: unsupported DLS technique");
   }
   //XBT_INFO("chunk size = %d \n",chunk_size);
   //getchar();
   return MIN(MAX(chunk_size, MIN_CHUNK_SIZE), remaining);
 }

/*calculate the number of chunks according to the current DLS algorithm */
int calculate_chunk_count(int num_tasks, int num_PE, int DLS, double h, double sigma, int PE_id)
{
    int remaining = num_tasks;
    int chunk_size = 1;
    int count = 0;

    switch(DLS)
    {
        case 0:
            //static chunking
            count = num_PE;
            break;
        case 1:
            //self scheduling
            count = num_tasks;
            break;
        case 3:
            //GSS
            chunk_size = ceil((double) remaining/num_PE);
            for (count = 0; remaining > 0; count++) {
                remaining = remaining - chunk_size ;
                chunk_size = ceil((double) remaining/num_PE);
            }
            break;
        case 4:
            //factoring
            while (remaining > 0) {
              chunk_size = calculate_chunk_size(num_tasks,num_PE,count,remaining,DLS,h,sigma,PE_id);
              remaining-=chunk_size;
              count++;
            }
            break;
            default:
            printf("Error: unsupported DLS technique");
    }

    return count;
}
