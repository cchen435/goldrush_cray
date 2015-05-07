GOLDRUSH_PATH = /home/cchen/Projects/Hobbes/cray/goldrush_cray/
HOME_PATH_INC = /home/cchen/apps/include/
HOME_PATH_LIB = /home/cchen/apps/lib/
COOPSCHED_LINUX = /home/cchen/Projects/Hobbes/cray/coopsched/coopsched_linux/

CFLAGS = -I$(GOLDRUSH_PATH) -I$(HOME_PATH_INC) -I$(COOPSCHED_LINUX) -L$(HOME_PATH_LIB) \
		 -L$(COOPSCHED_LINUX) -L$(GOLDRUSH_PATH) -std=c99 -fopenmp \
		 -DUSE_MPI=1 -DUSE_GOLDRUSH=1 -lpapi -lpfm -lgoldrush -lcoopsched	

CC = mpicc
MPICC = mpicc

all: 
	$(CC) -o omp_simulation omp_simulation.c $(GOLDRUSH_PATH)libgoldrush.a $(CFLAGS)
	
clean:
	rm -f $(wildcard  $(execs))  *.a
