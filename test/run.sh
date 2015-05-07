export GR_DO_PHASE_PERFCTR=1
export GR_IS_SIMULATION=1
export GR_DO_SUSPEND=1
export NUM_NODES=1
export OMPI_COMM_WORLD_LOCAL_RANK=0
export GR_MIN_PHASE_LEN=10
export GR_DO_STUB=1

export GR_IPC_THRESHOLD=2000
export GR_L2MISS_THRESHOLD=2000

export GR_PERFCTR_EVENTS="PAPI_TOT_CYC;PAPI_TOT_INS;PAPI_L2_DCM"


#mpirun -np 2  ./bench_gtc_sith_gcc_debug input/A.txt 1 2
mpirun --allow-run-as-root -np 2  ./omp_simulation 20
