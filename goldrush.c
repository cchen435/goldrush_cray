/**
 * GoldRush runtime system for managing analytics running on compute node.
 * GoldRush consists of the following components:
 * - process suspend/resume
 * - online simulation phase monitoring
 * 
 * written by Fang Zheng (fzheng@cc.gatech.edu) 
 */
#include <stdint.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <signal.h>
#include <semaphore.h>
#include <mpi.h>
#include "goldrush.h"
#include "rdtsc.h"
#include "gr_internal.h"

#ifdef GR_HAVE_PERFCTR
#include "gr_perfctr.h"
#include "gr_monitor_buffer.h"
#include "gr_stub.h"
#endif

#include "gr_phase.h"

/* changed by Chao for kitten, using kitten scheduler API 
   for suspend operation 
*/
#ifdef USE_COOPSCHED
#include "coopsched.h"
#endif

#ifdef USE_COOPLINUX
#include "coopsched_linux.h"
#endif 

/* Macros and constants */
#define CHOSEN_SHM_METHOD DF_SHM_METHOD_SYSV

/* Global Variable */
int gr_app_id;
MPI_Comm gr_comm;
int gr_comm_rank;
int gr_comm_size;
int gr_local_rank;
int gr_local_size;
int is_simulation;

unsigned long int current_phase_file;
unsigned int current_phase_line;
uint64_t current_phase_start_time;
long long current_phase_perfctr_values[NUM_EVENTS];
int current_phase_id = 0;
int has_start_phase = 0;
int is_in_mainloop = 0;

int gr_do_phase_perfctr = 1;
int min_phase_length = 0; //2000083; // 1 ms for smoky
int gr_do_stub = 1;

#ifdef GR_HAVE_PERFCTR
gr_mon_buffer monitor_buffer;
gr_mon_buffer_t gr_monitor_buffer = &monitor_buffer;

typedef struct _contension_param {
	double ipc_threshold;
	double l2_miss_threshold;
} contension_param, *contension_param_t;

contension_param _param;
contension_param_t param = &_param;
#endif

#ifdef DEBUG_TIMING
/* dump timing results */
int my_rank;
FILE *log_file;
char log_file_name[30];
uint64_t t1, t2, t3, t4, t5, t6, t7, t8, t9, t10;
uint64_t total_time = 0;
uint64_t monitor_time = 0;
uint64_t perfctr_time = 0;
uint64_t suspend_time = 0;
uint64_t schedule_time = 0;
uint64_t stub_time = 0;
uint64_t resume_time = 0;
uint64_t phase_time = 0;
uint64_t call_count = 0;
#endif

/*
 * Set the applicaiton ID.
 */
void gr_set_application_id(int id)
{
    gr_app_id = id;
}

/*
 * Get the applicaiton ID.
 */
void gr_get_application_id(int *id)
{
    *id = gr_app_id;
}

/*
 * Initialize GoldRush runtime library. 
 * Called by both simulation and analysis.
 *
 * Parameter:
 *  comm: MPI communicator of the calling program
 *
 * Return 0 for success and -1 for error.
 */
int gr_init(MPI_Comm comm)
{
    gr_comm = comm;
    MPI_Comm_rank(comm, &gr_comm_rank);
    MPI_Comm_size(comm, &gr_comm_size);
    gr_local_rank = gr_get_local_rank();
    gr_local_size = gr_get_num_procs_per_node(comm);

#ifdef USE_COOPSCHED
	coopsched_init();
	fprintf(stderr, "coop init finished\n");
#endif

#ifdef USE_COOPLINUX
	coopsched_linux_init(50000000, 100000000, 100000000);
	fprintf(stderr, "coop_linux init finished\n");
#endif

    is_simulation = 0;
    if(getenv("GR_IS_SIMULATION") != NULL) {
        is_simulation = 1;
    }
 
 #ifdef GR_HAVE_PERFCTR
    char *do_phase_perfctr_str = getenv("GR_DO_PHASE_PERFCTR");
    if(do_phase_perfctr_str != NULL) {
        int d = atoi(do_phase_perfctr_str);
        gr_do_phase_perfctr = (d == 0) ? 0:1;
    }

    if(gr_do_phase_perfctr) {
        gr_perfctr_init(gr_comm_rank);
        gr_perfctr_start(gr_comm_rank);
	}

	char *ipc_threshold_str = getenv("GR_IPC_THRESHOLD");
	if (ipc_threshold_str) {
		param->ipc_threshold = (double) atoi(ipc_threshold_str);
	} else {
		param->ipc_threshold = 1;
	}

	char *l2_miss_threshold_str = getenv("GR_L2MISS_THRESHOLD");
	if (l2_miss_threshold_str) {
		param->l2_miss_threshold = (double) atoi (l2_miss_threshold_str);
	} else {
		param->l2_miss_threshold = 10;
	}
#endif

    if(!is_simulation) { // analytics, no more work need to be done. 
        return 0;
    }

    // simulation specific initialization

    // phase performance history buffer
    // Chao: each MPI process will create a phase buffer to
    // record the length of the phase
    if(gr_create_global_phases(GR_DEFAULT_NUM_PHASES)) {
        exit(-1);
    }

    // threashold value, detemine whether the phase should be used for analysis
    char *min_phase_str = getenv("GR_MIN_PHASE_LEN");
    if(min_phase_str != NULL) {
        min_phase_length = atoi(min_phase_str);
    }

    // Chao: gr_do_stub is going to monitor the performance of simulation 
    // to decide whether run the analysis to avoid interference. not using
    // it currently
    char *gr_do_stub_str = getenv("GR_DO_STUB");
    if(gr_do_stub_str != NULL) {
        gr_do_stub = atoi(gr_do_stub_str);
    }
#ifdef DEBUG_TIMING
    my_rank = gr_comm_rank;
    sprintf(log_file_name, "timestamp.%d\0", my_rank);
    log_file = fopen(log_file_name, "w");
    if(!log_file) {
        fprintf(stderr, "cannot open file %s\n", log_file_name);
    }
#endif

    return 0;
}

/*
 * Finalize GoldRush runtime library.
 * Called by both simulation and analysis.
 *
 * Return 0 for success and -1 for error.
 */
int gr_finalize()
{
    gr_destroy_global_phases();
    gr_destroy_opened_files();

    if(!is_simulation) { // analytics
        return 0;
    }

    // simulation only

#ifdef GR_HAVE_PERFCTR
    if(gr_do_stub) {
        gr_stub_finalize();
    }
#endif

#ifdef DEBUG_TIMING
    MPI_Barrier(gr_comm);
    fprintf(log_file, "rank\tcall_count\ttotal\tmonitor\tsuspend\tschedule\tresume\tstub\n");
    fprintf(log_file, "%d\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n", 
        my_rank, call_count, total_time, monitor_time, suspend_time, schedule_time, resume_time, stub_time);
    fprintf(log_file, "phase_time:\t%lu\n", phase_time);
    fprintf(log_file, "\nTiming\n");
    gr_print_phases(log_file);
    fclose(log_file);
#endif

#ifdef GR_HAVE_PERFCTR
    gr_perfctr_finalize(gr_comm_rank);
#endif
#ifdef USE_COOPSCHED
        coopsched_deinit();
#endif

#ifdef USE_COOPLINUX
        coopsched_linux_deinit();
#endif

	return 0;
}

/* Public API used by simulation code */ 

/*
 * Mark the start of mainloop
 */
int gr_mainloop_start()
{
#ifdef USE_COOPSCHED
    coopsched_init_task(0);
#endif
    is_in_mainloop = 1;
    return 0;
}

/*
 * Mark the end of mainloop
 */
int gr_mainloop_end()
{
    is_in_mainloop = 0;

    /**
     * if worker process (not leader process) yielding CPU;
     * not check the length of phase in gr_mainloop_end
     */
	if (!gr_is_main_thread()) {
        /* check whether the idle length is long enough. */
#ifdef USE_COOPSCHED
        coopsched_yield_cpu_to(0);
#endif

#ifdef USE_COOPLINUX
        coopsched_linux_yield();
#endif
	}

    return 0;
}

/**
 * an wrap to gr_pahse_start using filename as an input
 */
int gr_phase_start_s(char *filename, unsigned int line) 
{
    /**
     * here using a hash function to hash the file name
     * into a unsigned long
     */
    //unsigned long int file = hash(filename);
    
    int fd = gr_open_file(filename);
    if (fd == -1) {
        fprintf(stderr, "failed to get file identifier for (%s), cannot estimate the length.\n", filename);
    } else {
        gr_phase_start(fd, line);
    }
    
    return 0;
}


/*
 * Mark the start of a phase. 
 *
 * Parameter:
 *  file: an integer identifying a source file
 *  line: an integer identifying line number in source file
 *
 * Return 0 for success and -1 for error. 
 */
int gr_phase_start(unsigned long int file, unsigned int line)
{
#ifdef DEBUG_TIMING
    t1 = rdtsc();
#endif

    // estimate the length of the current phase based on history info
    // skip small phases
    gr_phase_perf_t p_perf=NULL;
    gr_phase_t p = gr_find_phase(file, line, &p_perf); // make a guess
    int should_run = 0;

#if PRINT_AVG_LEN
	if(p_perf && gr_is_main_thread())
		fprintf(stdout, "**** average phase length ****\n\t %d \n*****************************\n", p_perf->avg_length);
#endif

	// find a phase with average length larger than minimal requirement
    if(p && p_perf && p_perf->avg_length != 0 && p_perf->avg_length >= min_phase_length) {

		long long *window = gr_monitor_buffer->perfctr_values;

		if (window[0] != 0) {
			double ipc = window[1]/window[0];
			double l2_miss_rate = window[2]/window[0] * 1000;

			if (ipc > param->ipc_threshold && \
					l2_miss_rate < param->l2_miss_threshold) {
				should_run = 1;
			}
		} else {
			should_run = 1;
		}
    }
    current_phase_file = file;
    current_phase_line = line;

#ifdef DEBUG_TIMING
    t2 = rdtsc();
#endif

    // worker threads yield cpu the analysis process
    if(should_run && !gr_is_main_thread()) {
#ifdef USE_COOPSCHED
        coopsched_yield_cpu_to(0);
#endif

#ifdef USE_COOPLINUX
        coopsched_linux_yield();
#endif
		return 0;
    }

	// main threads only
#ifdef DEBUG_TIMING
    t3 = rdtsc();
#endif

#ifdef GR_HAVE_PERFCTR
    if(gr_do_phase_perfctr) {
        gr_perfctr_read(current_phase_perfctr_values);
    }
#endif

    current_phase_start_time = rdtsc();

#ifdef DEBUG_TIMING
    t4 = rdtsc();
#endif

    if(gr_do_stub) {
        gr_stub_phase_start(current_phase_perfctr_values);
    }

#ifdef DEBUG_TIMING
    t5 = rdtsc();
#endif
    has_start_phase = 1;
    return 0;        
}

/**
 * a wraper to gr_phase_end, with parameter file as a string
 */
int gr_phase_end_s(char *filename, unsigned int line)
{

#ifdef USE_COOPSCHED
    coopsched_init_task(0);
#endif

    int fd = gr_open_file(filename);
    if (fd == -1) {
        fprintf(stderr, "failed to get file identifier, cannot estimate the length. \n");
    } else {
        gr_phase_end(fd, line);
    }
    return 0;
}
/*
 * Mark the end of a phase. It must match a gr_phase_start() call.
 *
 * Parameter:
 *  file: an integer identifying a source file
 *  line: an integer identifying line number in source file
 *
 * Return 0 for success and -1 for error. 
 */
int gr_phase_end(unsigned long int file, unsigned int line)
{
  if(has_start_phase) {

#ifdef DEBUG_TIMING
    t6 = rdtsc();
#endif

    uint64_t end_cycle = rdtsc();

    long long end_perfctr_values[NUM_EVENTS];

#ifdef GR_HAVE_PERFCTR
// optimize out
    if(gr_do_phase_perfctr) {
        gr_perfctr_read(end_perfctr_values);
    }
#endif

#ifdef DEBUG_TIMING
    t7 = rdtsc();
#endif

// optimize out
    if(gr_do_stub) {
        // disable the timer
        gr_stub_phase_end();
    }

#ifdef DEBUG_TIMING
    t8 = rdtsc();
    t9 = rdtsc();
#endif

    // update phase history
    int p_index = gr_get_phase(current_phase_file, 
                               current_phase_line,
                               file, 
                               line
                              );
    if(p_index == -1) {
        return -1;
    }
    uint64_t length = end_cycle - current_phase_start_time;

#ifdef GR_HAVE_PERFCTR
// optimize out
    if(gr_do_phase_perfctr) {
        int j;
        for(j = 0; j < NUM_EVENTS; j ++) {
            end_perfctr_values[j] -= current_phase_perfctr_values[j];
        }
    }
#endif
    gr_update_phase(p_index, length, end_perfctr_values);

#ifdef DEBUG_TIMING
    t10 = rdtsc();

    if(is_in_mainloop) {
        phase_time += t6 - t5;
        total_time += (t5-t1) + (t10-t6);
        monitor_time += (t4-t3) + (t7-t6);
        stub_time += (t5-t4) + (t8-t7);
        schedule_time += t2 - t1;
        resume_time += t3 - t2;
        suspend_time += t9 - t8;
        call_count ++;
    }
#endif
    has_start_phase = 0;
    return 0;
  }
  return -1;
}

