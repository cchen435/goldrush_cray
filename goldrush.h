#ifndef _GOLDRUSH_H_
#define _GOLDRUSH_H_
/**
 * GoldRush runtime system for managing analytics running on compute node.
 * GoldRush consists of the following components:
 * - process suspend/resume
 * - online simulation phase monitoring
 * 
 * written by Fang Zheng (fzheng@cc.gatech.edu) 
 */
#ifdef __cplusplus
extern "C" {
#endif
 
#include <stdint.h> 
#include <sys/types.h>
#include <mpi.h>

#define GR_MAX_PROCS 32
 
/*
 * Initialize GoldRush runtime library. 
 * Called by both simulation and analysis.
 *
 * Parameter:
 *  comm: MPI communicator of the calling program
 *
 * Return 0 for success and -1 for error.
 */
int gr_init(MPI_Comm comm); 

/*
 * Finalize GoldRush runtime library.
 * Called by both simulation and analysis.
 *
 * Return 0 for success and -1 for error.
 */
int gr_finalize(); 

/*
 * Set the applicaiton ID.
 */
void gr_set_application_id(int id);

/*
 * Get the applicaiton ID.
 */
void gr_get_application_id(int *id);

/* Public API used by simulation code */ 

/*
 * Mark the start of mainloop
 */
int gr_mainloop_start();

/*
 * Mark the end of mainloop
 */
int gr_mainloop_end();

/*
 * Mark the start of a phase. 
 *
 * Parameter:
 *  file: an integer identifying a source file
 *  line: an integer identifying line number in source file
 *
 * Return 0 for success and -1 for error. 
 */
int gr_phase_start(unsigned long int file, unsigned int line);

/*
 * Mark the end of a phase. It must match a gr_phase_start() call.
 *
 * Parameter:
 *  file: an integer identifying a source file
 *  line: an integer identifying line number in source file
 *
 * Return 0 for success and -1 for error. 
 */
int gr_phase_end(unsigned long int file, unsigned int line);


/* Fortran API */

int gr_init_(MPI_Fint *comm);

int gr_finalize_();

void gr_set_application_id_(int *id);

void gr_get_application_id_(int *id);

int gr_mainloop_start_();

int gr_mainloop_end_();

int gr_phase_start_(unsigned long int *file, unsigned int *line);

int gr_phase_end_(unsigned long int *file, unsigned int *line);


#ifdef __cplusplus
}
#endif

#endif
