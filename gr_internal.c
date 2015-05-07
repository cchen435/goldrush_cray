#include <stdio.h> 
#include <stdint.h> 
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <mpi.h>
#include <omp.h>

#include "gr_internal.h"

extern int gr_app_id;
extern MPI_Comm gr_comm;
extern int gr_comm_rank;
extern int gr_comm_size;

/* Global Variables */

/* cache a copy of this */
int gr_num_nodes = 0;

/*
 * Get the number of processes on each node
 */
int gr_get_num_procs_per_node(MPI_Comm comm)
{
    // TODO: get it from MPI runtime, e.g., OMPI_COMM_WORLD_LOCAL_SIZE 
    if(gr_num_nodes == 0) {
        char *temp_string = getenv("NUM_NODES");
        if(!temp_string) {
            fprintf(stderr, "Error: env variable NUM_NODES not set.\n");
            exit(-1);
        }
        gr_num_nodes = atoi(temp_string);
    }
    int comm_size;
    MPI_Comm_size(comm, &comm_size);
    int num_procs_per_node = comm_size / gr_num_nodes;
    return num_procs_per_node;
}

/*
 * Get the pids of processes on each node.
 * This is a collective call and every process within comm communicator
 * should call this function.
 */
int gr_get_pids(pid_t *pid)
{
    int local_rank = gr_get_local_rank();
    pid_t my_pid = getpid();
    pid[local_rank] = my_pid;
    return 0;
}

/*
 * Get application id of the sender for the specified data group.
 */
int gr_get_sender_app_id(char *data_group_name)
{
    // TODO: get this from shm transport
    return 0;
}

/* Check whether the thread is the main thread */
int gr_is_main_thread() {
    int tid = omp_get_thread_num();
    return (tid==0);
}

/*
 * Test if the calling process is the local leader on the local node.
 * Return 1 for yes and 0 for no.
 */
int gr_is_local_leader()
{
    int local_rank = gr_get_local_rank();
//  fprintf(stderr, "rank %d local %d\n", gr_comm_rank, local_rank);
    return (local_rank == 0) && (getenv("GR_IS_SIMULATION") != NULL);
//    return (local_rank == 0);
#if 0
    int local_universe_rank = atoi(getenv("OMPI_COMM_WORLD_NODE_RANK"));
    return local_universe_rank == 0;
#endif
} 

/*
 * Get local rank on node
 */
int gr_get_local_rank() 
{
    int local_rank;
#ifdef GR_IS_TITAN
    int num_procs_per_node = gr_get_num_procs_per_node(gr_comm);
    local_rank = gr_comm_rank % num_procs_per_node;
#else
    local_rank = atoi(getenv("OMPI_COMM_WORLD_LOCAL_RANK"));
#endif
    return local_rank;
}
