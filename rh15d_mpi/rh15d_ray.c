#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "rh.h"
#include "atom.h"
#include "atmos.h"
#include "geometry.h"
#include "spectrum.h"
#include "background.h"
#include "statistics.h"
#include "error.h"
#include "inputs.h"
#include "parallel.h"
#include "io.h"

#define COMMENT_CHAR    "#"
#define RAY_INPUT_FILE  "ray.input"

/* --- Function prototypes --                          -------------- */
void init_ncdf_ray(void);
void writeRay(void);
void close_ncdf_ray(void);

/* --- Global variables --                             -------------- */

enum Topology topology = ONE_D_PLANE;

Atmosphere atmos;
Geometry geometry;
Spectrum spectrum;
ProgramStats stats;
InputData input;
NCDF_Atmos_file infile;
CommandLine commandline;
char messageStr[MAX_MESSAGE_LENGTH];
BackgroundData bgdat;
MPI_data  mpi;
IO_data   io;
IO_buffer iobuf;

/* ------- begin -------------------------- rhf1d.c ----------------- */

int main(int argc, char *argv[])
{
  bool_t analyze_output, equilibria_only, run_ray, exit_on_EOF;
  int    niter, nact, i, Nspect, Nread, Nrequired,
         checkPoint, save_Nrays, *wave_index = NULL;
  double muz, save_muz, save_mux, save_muy, save_wmu;
  Atom  *atom;
  FILE  *fp_ray;

  char  inputLine[MAX_LINE_SIZE];


  /* --- Set up MPI ----------------------             -------------- */
  initParallel(&argc, &argv, run_ray=FALSE);

  setOptions(argc, argv);
  getCPU(0, TIME_START, NULL);
  SetFPEtraps();

  /* Direct log stream into MPI log files */
  mpi.main_logfile     = commandline.logfile;
  commandline.logfile  = mpi.logfile;

  /* --- Read input data and initialize --             -------------- */
  readInput();
  spectrum.updateJ = FALSE;

  getCPU(1, TIME_START, NULL);
  init_ncdf_atmos(&atmos, &geometry, &infile);

  /* Find out the work load for each process */
  distribute_jobs();


  /* --- Read ray.input --                            --------------- */
  /* --- Read direction cosine for ray --              -------------- */
  if ((fp_ray = fopen(RAY_INPUT_FILE, "r")) == NULL) {
    sprintf(messageStr, "Unable to open inputfile %s", RAY_INPUT_FILE);
    Error(ERROR_LEVEL_2, argv[0], messageStr);
  }
  
  getLine(fp_ray, COMMENT_CHAR, inputLine, exit_on_EOF=TRUE);
  Nread = sscanf(inputLine, "%lf", &muz);
  checkNread(Nread, Nrequired=1, argv[0], checkPoint=1);

  if (muz <= 0.0  ||  muz > 1.0) {
    sprintf(messageStr,
	    "Value of muz = %f does not lie in interval <0.0, 1.0]\n", muz);
    Error(ERROR_LEVEL_2, argv[0], messageStr);
  }

  /* --- read how many points to write detailed S, chi, eta, etc ---- */
  Nread = fscanf(fp_ray, "%d", &Nspect);
  checkNread(Nread, 1, argv[0], checkPoint=2);
  io.ray_nwave_sel = Nspect;

   /* --- Read wavelength indices for which chi and S are to be
       written out for the specified direction --    -------------- */

  if (Nspect > 0) {
    io.ray_wave_idx = (int *) malloc(Nspect * sizeof(int));
    Nread = 0;
    while (fscanf(fp_ray, "%d", &io.ray_wave_idx[Nread]) != EOF) Nread++;
    checkNread(Nread, Nspect, argv[0], checkPoint=3);
    fclose(fp_ray);

    wave_index = io.ray_wave_idx;
  }

  /* --- Save geometry values to change back after --    ------------ */
  save_Nrays = atmos.Nrays;   save_wmu = geometry.wmu[0];
  save_muz = geometry.muz[0]; save_mux = geometry.mux[0]; save_muy = geometry.muy[0];



  /* Main loop over tasks */
  for (mpi.task = 0; mpi.task < mpi.Ntasks; mpi.task++) {

    /* Indices of x and y */
    mpi.ix = mpi.taskmap[mpi.task + mpi.my_start][0];
    mpi.iy = mpi.taskmap[mpi.task + mpi.my_start][1];
   
    /* Printout some info */
    sprintf(messageStr,
      "Process %3d: --- START task %3ld [of %ld], (xi,yi) = (%3d,%3d)\n",
       mpi.rank, mpi.task+1, mpi.Ntasks, mpi.xnum[mpi.ix], mpi.ynum[mpi.iy]);
    fprintf(mpi.main_logfile, messageStr);
    Error(MESSAGE, "main", messageStr);

    /* Read atmosphere column */
    readAtmos_ncdf(mpi.xnum[mpi.ix],mpi.ynum[mpi.iy], &atmos, &geometry, &infile);


    if (atmos.Stokes) Bproject();

    /* --- Run only once --                                  --------- */
    if (mpi.task == 0) {
      readAtomicModels();   
      readMolecularModels();

      SortLambda();
      
      bgdat.write_BRS = FALSE;
      
      /* --- START stuff from initParallelIO, just getting the needed parts --- */
      /* In this mode, only background, indata and ray files are written */
      init_Background();
      init_ncdf_indata();
      init_ncdf_ray();
      
      /* Get file position of atom files (to re-read collisions) */
      io.atom_file_pos = (long *) malloc(atmos.Nactiveatom * sizeof(long));
      mpi.zcut_hist    = (int *)    calloc(mpi.Ntasks , sizeof(int));

      for (nact = 0; nact < atmos.Nactiveatom; nact++) {
	atom = atmos.activeatoms[nact];
	io.atom_file_pos[nact] = ftell(atom->fp_input);
      }

      /* Save StokesMode (before adjustStokesMode changes it...) */
      mpi.StokesMode_save = input.StokesMode;
      mpi.single_log       = FALSE;  
  
      /* Allocate some mpi. arrays */
      mpi.niter       = (int *)    calloc(mpi.Ntasks , sizeof(int));
      mpi.convergence = (int *)    calloc(mpi.Ntasks , sizeof(int));
      mpi.zcut_hist   = (int *)    calloc(mpi.Ntasks , sizeof(int));
      mpi.dpopsmax    = (double *) calloc(mpi.Ntasks , sizeof(double));
      mpi.dpopsmax_hist = matrix_double(mpi.Ntasks, input.NmaxIter);

      mpi.zcut_hist[mpi.task] = mpi.zcut;
  
      /* Fill mpi.niter with ones, to avoid problems with crashes on 1st iteration */
      for (i=0; i < mpi.Ntasks; i++) mpi.niter[i] = 1;
    
      /* --- END of stuff from initParalleIO ---*/

    } else {
      /* Update quantities that depend on atmosphere and initialise others */
      UpdateAtmosDep();
    }

    /* --- Calculate background opacities --             ------------- */
    Background_p(analyze_output=FALSE, equilibria_only=FALSE);

    getProfiles();
    initSolution_p();
    initScatter();


    getCPU(1, TIME_POLL, "Total Initialize");


    
    /* --- Solve radiative transfer for active ingredients -- --------- */
    Iterate_p(input.NmaxIter, input.iterLimit);

    /* Treat odd cases as a crash */
    if (isnan(mpi.dpopsmax[mpi.task]) || isinf(mpi.dpopsmax[mpi.task]) || 
	(mpi.dpopsmax[mpi.task] < 0) || ((mpi.dpopsmax[mpi.task] == 0) && (input.NmaxIter > 0)))
      mpi.stop = TRUE;    
    
    /* In case of crash, write dummy data and proceed to next task */
    if (mpi.stop) {
      sprintf(messageStr,
	      "Process %3d: *** SKIP  task %3ld (crashed after %d iterations)\n",
	      mpi.rank, mpi.task+1, mpi.niter[mpi.task]);
      fprintf(mpi.main_logfile, messageStr);
      Error(MESSAGE, "main", messageStr);

      mpi.ncrash++;
      mpi.stop = FALSE;
      mpi.dpopsmax[mpi.task] = 0.0;
      mpi.convergence[mpi.task] = -1;

      continue;
    }


    if (mpi.convergence[mpi.task]) {
      
      /* Lambda iterate mean radiation field */
      adjustStokesMode();
      niter = 0;
      while (niter < input.NmaxScatter) {
        if (solveSpectrum(FALSE, FALSE) <= input.iterLimit) break;
        niter++;
      }
      
      /* Redefine geometry just for this ray */
      atmos.Nrays     = geometry.Nrays = 1;
      geometry.muz[0] = muz;
      geometry.mux[0] = sqrt(1.0 - SQ(geometry.muz[0]));
      geometry.muy[0] = 0.0;
      geometry.wmu[0] = 1.0;
      
      /* --- Solve radiative transfer for ray --           -------------- */
      solveSpectrum(FALSE, FALSE);

      /* --- Write emergent spectrum to output file --     -------------- */
      writeRay();

      /* Put back previous values for geometry */
      atmos.Nrays     = geometry.Nrays = save_Nrays;
      geometry.muz[0] = save_muz;
      geometry.mux[0] = save_mux;
      geometry.muy[0] = save_muy;
      geometry.wmu[0] = save_wmu;  
    }
   
    /* Printout some info, finished iter */
    if (mpi.convergence[mpi.task]) {
      sprintf(messageStr,
       "Process %3d: *** END   task %3ld iter, iterations = %3d, CONVERGED\n",
       mpi.rank, mpi.task+1, mpi.niter[mpi.task]);
      mpi.nconv++;
    } else {
      sprintf(messageStr,
       "Process %3d: *** END   task %3ld iter, iterations = %3d, NO convergence\n",
       mpi.rank, mpi.task+1, mpi.niter[mpi.task]);
      mpi.nnoconv++;
    }

  } /* End of main task loop */


  /* --- Write output to indata file --- */
  sprintf(messageStr, "Process %3d: --- START output\n", mpi.rank);
  fprintf(mpi.main_logfile, messageStr);
  Error(MESSAGE, "main", messageStr);
  
  writeMPI_all();
  writeAtmos_all();
  
  sprintf(messageStr, "Process %3d: *** END output\n", mpi.rank);
  fprintf(mpi.main_logfile, messageStr);
  Error(MESSAGE, "main", messageStr);

  /* --- Stuff that was on closeParallelIO --- */
  close_ncdf_indata();
  close_ncdf_atmos(&atmos, &geometry, &infile);
  close_Background();
  free(io.atom_file_pos);
  /* --- END of stuff from closeParallelIO ---*/
  
  close_ncdf_ray();
  
  
  
  /* Frees from memory stuff used for job control */
  finish_jobs();

  sprintf(messageStr,
	  "*** Job ending. Total %ld 1-D columns: %ld computed, %ld skipped.\n%s",
	  mpi.Ntasks, mpi.nconv, mpi.ncrash,
	  "*** RH_ray finished gracefully.\n");	  
  if (mpi.rank == 0) fprintf(mpi.main_logfile, messageStr);
  Error(MESSAGE,"main",messageStr);

  printTotalCPU();
  MPI_Finalize();

  return 0;
}
/* ------- end ---------------------------- rhf1d.c ----------------- */
