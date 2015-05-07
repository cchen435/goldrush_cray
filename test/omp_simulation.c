/* Hobbes Project - Cooperative Scheduling Interface
 * (c) 2015, Oscar Mondragon <omondrag@cs.unm.edu>
 */


/*Simple Analytics*/
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if USE_GOLDRUSH
#include "goldrush.h"
#endif
#include <stdlib.h>

#define SCALE 10000
#define ARRINIT 2000

//pi_digits is taken from http://www.codecodex.com/wiki/Calculate_digits_of_pi

void pi_digits(int digits) {
    int carry = 0;
    int arr[digits + 1];
    for (int i = 0; i <= digits; ++i)
        arr[i] = ARRINIT;
    for (int i = digits; i > 0; i-= 14) {
        int sum = 0;
        for (int j = i; j > 0; --j) {
            sum = sum * j + SCALE * arr[j];
            arr[j] = sum % (j * 2 - 1);
            sum /= j * 2 - 1;
        }
        //printf("%04d", carry + sum / SCALE);
        carry = sum % SCALE;
    }

}

int
omp_calculate_pidigits(){

    int nthreads, tid;
    
    /* Fork a team of threads giving them their own copies of variables */
#pragma omp parallel private(nthreads, tid)
    {
#if USE_GOLDRUSH
    gr_phase_end_s (__FILE__, __LINE__);
#endif

    /* Obtain thread number */
    tid = omp_get_thread_num();
	
    if(tid != 0)
        pi_digits(2000);

    /* Only master thread does this */
    if (tid == 0) 
    {
        nthreads = omp_get_num_threads();
        //printf("Number of threads = %d\n", nthreads);
    }
#if USE_GOLDRUSH
    gr_phase_start_s (__FILE__, __LINE__);
#endif

    }  /* All threads join master thread and disband */

    //printf("Start of sequential section\n");
    pi_digits(1000);
    pi_digits(1000);
    //printf("End of sequential section\n");

    return 0;
}



int main (int argc, char *argv[]) 
{
    if(argc < 2){
        printf("usage: ./omp_simple_simulation <times>\n");
        return -1;
    }

    int times = atoi(argv[1]);
    int i = 0;    
#if USE_MPI
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
#endif

#if USE_GOLDRUSH
    gr_init (MPI_COMM_WORLD);
#endif

#if USE_GOLDRUSH
    gr_mainloop_start();
#endif
    for( i = 0; i < times; i++){
		printf("(%d) Simulation: Calculating pi_digits\n", i);
        omp_calculate_pidigits();
		printf("(%d) pi_digits done\n", i);
    }
#if USE_GOLDRUSH
	gr_mainloop_end();
#endif 

#if USE_GOLDRUSH
    gr_finalize ();
#endif
	
#if USE_MPI
    MPI_Finalize();
#endif
    printf("END OF TEST\n");
}
