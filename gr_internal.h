#ifndef _GR_INTERNAL_H_
#define _GR_INTERNAL_H_

#include <stdint.h> 
#include <sys/types.h>
#include <semaphore.h>
#include <mpi.h>
#include "df_shm.h"
#include "goldrush.h"

#define GR_MAX_NUM_FILES 5
#define GR_MAX_NUM_RECEIVERS 5
#define GR_MAX_NUM_SENDERS 5
#define GR_MAX_NUM_PROCS 32
#define PAGE_SIZE 4096

/*
 * Get the number of processes on each node
 */
int gr_get_num_procs_per_node(MPI_Comm comm);

/*
 * Get the pids of processes on each node.
 * This is a collective call and every process within comm communicator
 * should call this function.
 */
int gr_get_pids(pid_t *pid);

/*
 * Get application id of the sender for the specified data group.
 */
int gr_get_sender_app_id(char *data_group_name);

/* check whether the thread is the main thread */
int gr_is_main_thread();

/*
 * Test if the calling process is the local leader on the local node.
 * Return 1 for yes and 0 for no.
 */
int gr_is_local_leader();

/*
 * Get local rank on node
 */
int gr_get_local_rank();


#endif
