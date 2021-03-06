/* ------- file: -------------------------- writeindata_p.c ------------

       Version:       rh2.0, 1.5-D plane-parallel
       Author:        Tiago Pereira (tiago.pereira@nasa.gov)
       Last modified: Thu Dec 30 22:58:15 2010 --

       --------------------------                      -----------RH-- */

/* --- Writes input data (including input, atmos, geom) to output file */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "rh.h"
#include "atom.h"
#include "atmos.h"
#include "geometry.h"
#include "error.h"
#include "inputs.h"
#include "parallel.h"
#include "io.h"


/* --- Function prototypes --                          -------------- */


/* --- Global variables --                             -------------- */

extern Atmosphere atmos;
extern Geometry geometry;
extern InputData input;
extern char messageStr[];
extern Input_Atmos_file infile;
extern MPI_data mpi;
extern IO_data io;


/* ------- begin --------------------------   init_hdf5_indata.c  --- */
void init_hdf5_indata(void) {
  /* Wrapper to find out if we should use old file or create new one */

  if (input.p15d_rerun) init_hdf5_indata_existing(); else init_hdf5_indata_new();

  return;
}
/* ------- end   --------------------------   init_hdf5_indata.c  --- */


/* ------- begin --------------------------   init_hdf5_indata.c  --- */
void init_hdf5_indata_new(void)
/* Creates the file for the input data */
{
  const char routineName[] = "init_hdf5_indata_new";
  unsigned int *tmp;
  int     i, PRD_angle_dep;
  double  *eweight, *eabund, *tmp_double;;
  /* This value is harcoded for efficiency.
     Maximum number of iterations ever needed */
  int     NMaxIter = 1500;
  hid_t   plist, ncid, file_dspace, ncid_input, ncid_atmos, ncid_mpi;
  hid_t   id_x, id_y, id_z, id_n, id_tmp;
  hsize_t dims[4];
  bool_t   XRD;
  char    startJ[MAX_LINE_SIZE], StokesMode[MAX_LINE_SIZE], angleSet[MAX_LINE_SIZE];

  /* Create the file  */
  if (( plist = H5Pcreate(H5P_FILE_ACCESS )) < 0) HERR(routineName);
  if (( H5Pset_fapl_mpio(plist, mpi.comm, mpi.info) ) < 0) HERR(routineName);
  if (( ncid = H5Fcreate(INPUTDATA_FILE, H5F_ACC_TRUNC, H5P_DEFAULT,
                         plist) ) < 0) HERR(routineName);
  if (( H5Pclose(plist) ) < 0) HERR(routineName);

  /* Create groups */
  if (( ncid_input = H5Gcreate(ncid, "/input", H5P_DEFAULT, H5P_DEFAULT,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( ncid_atmos = H5Gcreate(ncid, "/atmos", H5P_DEFAULT, H5P_DEFAULT,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( ncid_mpi = H5Gcreate(ncid, "/mpi", H5P_DEFAULT, H5P_DEFAULT,
                               H5P_DEFAULT) ) < 0) HERR(routineName);

  /* --- Definitions for the root group --- */
  /* dimensions as attributes */
  if (( H5LTset_attribute_int(ncid, "/", "nx", &mpi.nx, 1) ) < 0)
      HERR(routineName);
  if (( H5LTset_attribute_int(ncid, "/", "ny", &mpi.ny, 1) ) < 0)
      HERR(routineName);
  if (( H5LTset_attribute_int(ncid, "/", "nz", (int *) &infile.nz, 1 )) < 0)
      HERR(routineName);
  /* attributes */
  if (( H5LTset_attribute_string(ncid, "/", "atmosID", atmos.ID)) < 0)
    HERR(routineName);
  if (( H5LTset_attribute_string(ncid, "/", "rev_id", mpi.rev_id) ) < 0)
    HERR(routineName);
  /* --- dimension datasets, all values set to zero --- */
  dims[0] = infile.nz;
  tmp_double = (double *) calloc(infile.nz , sizeof(double));
  if (( H5LTmake_dataset(ncid, ZOUT_NAME, 1, dims, H5T_NATIVE_DOUBLE,
                         tmp_double) ) < 0)  HERR(routineName);
  free(tmp_double);
  if (( id_z = H5Dopen2(ncid, ZOUT_NAME, H5P_DEFAULT)) < 0) HERR(routineName);
  /* For compatibility with netCDF readers, only use dataset as dimension */
  if (( H5LTset_attribute_string(ncid, ZOUT_NAME, "NAME",
                                 NETCDF_COMPAT) ) < 0) HERR(routineName);

  /* --- Definitions for the INPUT group --- */
  /* attributes */
  if ( atmos.NPRDactive > 0)
    PRD_angle_dep = input.PRD_angle_dep;
  else
    PRD_angle_dep=0;

  XRD = (input.XRD  &&  atmos.NPRDactive > 0);

  if (( H5LTset_attribute_uchar(ncid_input, ".", "Magneto_optical",
          (unsigned char *) &input.magneto_optical, 1)) < 0) HERR(routineName);
  if (( H5LTset_attribute_uchar(ncid_input, ".", "PRD_angle_dep",
          (unsigned char *) &PRD_angle_dep, 1)) < 0) HERR(routineName);
  if (( H5LTset_attribute_uchar(ncid_input, ".", "XRD",
          (unsigned char *) &XRD, 1)) < 0) HERR(routineName);
  if (( H5LTset_attribute_uchar(ncid_input, ".", "Background_polarization",
          (unsigned char *) &input.backgr_pol, 1)) < 0) HERR(routineName);

  switch (input.startJ) {
  case UNKNOWN:
    strcpy(startJ, "Unknown");
    break;
  case LTE_POPULATIONS:
    strcpy(startJ, "LTE_POPULATIONS");
    break;
  case ZERO_RADIATION:
    strcpy(startJ, "ZERO_RADIATION");
    break;
  case OLD_POPULATIONS:
    strcpy(startJ, "OLD_POPULATIONS");
    break;
  case ESCAPE_PROBABILITY:
    strcpy(startJ, "ESCAPE_PROBABILITY");
    break;
  case NEW_J:
    strcpy(startJ, "NEW_J");
    break;
  case OLD_J:
    strcpy(startJ, "OLD_J");
    break;
  }
  if (( H5LTset_attribute_string(ncid_input, ".", "Start_J", startJ)) < 0)
    HERR(routineName);

  switch (input.StokesMode) {
  case NO_STOKES:
    strcpy(StokesMode, "NO_STOKES");
    break;
  case FIELD_FREE:
    strcpy(StokesMode, "FIELD_FREE");
    break;
  case POLARIZATION_FREE:
    strcpy(StokesMode, "POLARIZATION_FREE");
    break;
  case FULL_STOKES:
    strcpy(StokesMode, "FULL_STOKES");
    break;
  }
  if (( H5LTset_attribute_string(ncid_input, ".", "Stokes_mode",
                                 StokesMode) ) < 0) HERR(routineName);

  switch (atmos.angleSet.set) {
  case SET_VERTICAL:
    strcpy(angleSet, "SET_VERTICAL");
    break;
  case SET_GL:
    strcpy(angleSet, "SET_GL");
    break;
  case SET_A2:
    strcpy(angleSet, "SET_A2");
    break;
  case SET_A4:
    strcpy(angleSet, "SET_A4");
    break;
  case SET_A6:
    strcpy(angleSet, "SET_A6");
    break;
  case SET_A8:
    strcpy(angleSet, "SET_A8");
    break;
  case SET_B4:
    strcpy(angleSet, "SET_B4");
    break;
  case SET_B6:
    strcpy(angleSet, "SET_B6");
    break;
  case SET_B8:
    strcpy(angleSet, "SET_B8");
    break;
  case NO_SET:
    strcpy(angleSet, "NO_SET");
    break;
  }
  if (( H5LTset_attribute_string(ncid_input, ".", "Angle_set",
                                 angleSet) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_input, ".", "Atmos_file",
                                 input.atmos_input) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_input, ".", "Abundances_file",
                                 input.abund_input) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_input, ".", "Kurucz_PF_data",
                                 input.pfData) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_double(ncid_input, ".", "Iteration_limit",
                                 &input.iterLimit, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_double(ncid_input, ".", "PRD_Iteration_limit",
                              &input.PRDiterLimit, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "N_max_iter",
                              &input.NmaxIter, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "Ng_delay",
                              &input.Ngdelay, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "Ng_order",
                              &input.Ngorder, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "Ng_period",
                              &input.Ngperiod, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "PRD_N_max_iter",
                              &input.PRD_NmaxIter, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "PRD_Ng_delay",
                              &input.PRD_Ngdelay, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "PRD_Ng_order",
                              &input.PRD_Ngorder, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_input, ".", "PRD_Ng_period",
                              &input.PRD_Ngperiod, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_double(ncid_input, ".", "Metallicity",
                               &input.metallicity, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_double(ncid_input, ".", "Lambda_reference",
                                &atmos.lambda_ref, 1) ) < 0) HERR(routineName);

  /* --- Definitions for the ATMOS group --- */
  /* dimensions */
  if (( H5LTset_attribute_int(ncid_atmos, ".", "nelements",
                              &atmos.Nelem, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_atmos, ".", "nrays",
                              &geometry.Nrays, 1) ) < 0) HERR(routineName);
  /* --- dimension datasets --- */
  dims[0] = mpi.nx;
  if (( H5LTmake_dataset(ncid_atmos, X_NAME, 1, dims, H5T_NATIVE_DOUBLE,
                         geometry.xscale) ) < 0)  HERR(routineName);
  if (( id_x = H5Dopen2(ncid_atmos, X_NAME, H5P_DEFAULT)) < 0) HERR(routineName);
  dims[0] = mpi.ny;
  if (( H5LTmake_dataset(ncid_atmos, Y_NAME, 1, dims, H5T_NATIVE_DOUBLE,
                         geometry.yscale) ) < 0)  HERR(routineName);
  if (( id_y = H5Dopen2(ncid_atmos, Y_NAME, H5P_DEFAULT)) < 0) HERR(routineName);
  dims[0] = atmos.Nelem;
  tmp = (unsigned int *) calloc(atmos.Nelem , sizeof(unsigned int));
  if (( H5LTmake_dataset(ncid_atmos, ELEM_NAME, 1, dims, H5T_NATIVE_UINT,
                         tmp) ) < 0)  HERR(routineName);
  free(tmp);
  dims[0] = geometry.Nrays;
  tmp = (unsigned int *) calloc(geometry.Nrays , sizeof(unsigned int));
  if (( H5LTmake_dataset(ncid_atmos, RAY_NAME, 1, dims, H5T_NATIVE_UINT,
                         tmp) ) < 0)  HERR(routineName);
  free(tmp);
  /* For compatibility with netCDF readers, only use dataset as dimension */
  if (( H5LTset_attribute_string(ncid_atmos, ELEM_NAME, "NAME",
                                 NETCDF_COMPAT) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_atmos, RAY_NAME, "NAME",
                                 NETCDF_COMPAT) ) < 0) HERR(routineName);

  /* variables*/
  dims[0] = mpi.nx;
  dims[1] = mpi.ny;
  dims[2] = infile.nz;
  if (( file_dspace = H5Screate_simple(3, dims, NULL) ) < 0) HERR(routineName);
  if (( plist = H5Pcreate(H5P_DATASET_CREATE) ) < 0) HERR(routineName);
  if (( H5Pset_fill_value(plist, H5T_NATIVE_FLOAT, &FILLVALUE) ) < 0)
    HERR(routineName);
  if (( H5Pset_alloc_time(plist, H5D_ALLOC_TIME_EARLY) ) < 0) HERR(routineName);
  if (( H5Pset_fill_time(plist, H5D_FILL_TIME_ALLOC) ) < 0) HERR(routineName);
  if (( io.in_atmos_T = H5Dcreate(ncid_atmos, "temperature", H5T_NATIVE_FLOAT,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_T, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_T, id_y, 1)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_T, id_z, 2)) < 0) HERR(routineName);
  if (( H5LTset_attribute_float(ncid_atmos, "temperature", "_FillValue",
                                &FILLVALUE, 1) ) < 0) HERR(routineName);
  if (( io.in_atmos_vz = H5Dcreate(ncid_atmos, "velocity_z", H5T_NATIVE_FLOAT,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_vz, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_vz, id_y, 1)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_vz, id_z, 2)) < 0) HERR(routineName);
  if (( H5LTset_attribute_float(ncid_atmos, "velocity_z", "_FillValue",
                                &FILLVALUE, 1) ) < 0) HERR(routineName);
  if (( io.in_atmos_z = H5Dcreate(ncid_atmos, "height_scale", H5T_NATIVE_FLOAT,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_z, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_z, id_y, 1)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_atmos_z, id_z, 2)) < 0) HERR(routineName);
  if (( H5LTset_attribute_float(ncid_atmos, "height_scale", "_FillValue",
                                &FILLVALUE, 1) ) < 0) HERR(routineName);

  if (( H5Pclose(plist) ) < 0) HERR(routineName);
  if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);
  /* --- Write some data that does not depend on xi, yi, ATMOS group --- */
  /* arrays of number of elements */
  eweight = (double *) malloc(atmos.Nelem * sizeof(double));
  eabund = (double *) malloc(atmos.Nelem * sizeof(double));
  for (i=0; i < atmos.Nelem; i++) {
    eweight[i] = atmos.elements[i].weight;
    eabund[i] = atmos.elements[i].abund;
  }
  dims[0] = atmos.Nelem;
  if (( id_n = H5Dopen2(ncid_atmos, ELEM_NAME,
                        H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5LTmake_dataset(ncid_atmos, "element_weight", 1, dims,
                H5T_NATIVE_DOUBLE, eweight) ) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_atmos, "element_weight",
                          H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(id_tmp, id_n, 0)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  if (( H5LTmake_dataset(ncid_atmos, "element_abundance", 1, dims,
                H5T_NATIVE_DOUBLE, eabund) ) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_atmos, "element_abundance",
                          H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(id_tmp, id_n, 0)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  if (( H5Dclose(id_n) ) < 0) HERR(routineName);
  /* Not writing element_id for now
  dims[1] = strlen;
  if (( H5LTmake_dataset(ncid_atmos, "element_id", 2, dims,
                H5T_C_S1, eID) ) < 0) HERR(routineName);
  */
  free(eweight);
  free(eabund);

  dims[0] = geometry.Nrays;
  if (( id_n = H5Dopen2(ncid_atmos, RAY_NAME,
                        H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5LTmake_dataset(ncid_atmos, "muz", 1, dims,
              H5T_NATIVE_DOUBLE, geometry.muz) ) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_atmos, "muz", H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(id_tmp, id_n, 0)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  if (( H5LTmake_dataset(ncid_atmos, "wmu", 1, dims,
              H5T_NATIVE_DOUBLE, geometry.wmu) ) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_atmos, "wmu", H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(id_tmp, id_n, 0)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  if (( H5Dclose(id_n) ) < 0) HERR(routineName);

  /* attributes */
  if (( H5LTset_attribute_uchar(ncid_atmos, ".", "moving",
                   (unsigned char *) &atmos.moving, 1)) < 0) HERR(routineName);
  if (( H5LTset_attribute_uchar(ncid_atmos, ".", "stokes",
                   (unsigned char *) &atmos.Stokes, 1)) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_atmos, "temperature", "units",
                                 "K") ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_atmos,  "velocity_z", "units",
                                 "m s^-1") ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_atmos,  "height_scale", "units",
                                 "m") ) < 0) HERR(routineName);
  if (( H5LTset_attribute_string(ncid_atmos,  "element_weight", "units",
                                 "atomic_mass_units") ) < 0) HERR(routineName);
  if (( H5Dclose(id_x) ) < 0) HERR(routineName);
  if (( H5Dclose(id_y) ) < 0) HERR(routineName);

  /* --- Definitions for the MPI group --- */
  /* dimensions */
  if (( H5LTset_attribute_int(ncid_mpi, ".", "nprocesses",
                              &mpi.size, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_mpi, ".", "niterations",
                              &NMaxIter, 1) ) < 0) HERR(routineName);
  /* --- dimension datasets --- */
  dims[0] = mpi.nx;
  if (( H5LTmake_dataset(ncid_mpi, X_NAME, 1, dims, H5T_NATIVE_DOUBLE,
                         geometry.xscale) ) < 0)  HERR(routineName);
  if (( id_x = H5Dopen2(ncid_mpi, X_NAME, H5P_DEFAULT)) < 0) HERR(routineName);
  dims[0] = mpi.ny;
  if (( H5LTmake_dataset(ncid_mpi, Y_NAME, 1, dims, H5T_NATIVE_DOUBLE,
                         geometry.yscale) ) < 0)  HERR(routineName);
  if (( id_y = H5Dopen2(ncid_mpi, Y_NAME, H5P_DEFAULT)) < 0) HERR(routineName);
  dims[0] = NMaxIter;
  tmp = (unsigned int *) calloc(NMaxIter , sizeof(unsigned int));
  if (( H5LTmake_dataset(ncid_mpi, IT_NAME, 1, dims, H5T_NATIVE_UINT,
                         tmp) ) < 0)  HERR(routineName);
  free(tmp);
  /* For compatibility with netCDF readers, only use dataset as dimension */
  if (( H5LTset_attribute_string(ncid_mpi, IT_NAME, "NAME",
                                 NETCDF_COMPAT) ) < 0) HERR(routineName);
  /* variables*/
  dims[0] = mpi.nx;
  if (( H5LTmake_dataset(ncid_mpi, XNUM_NAME, 1, dims,
                H5T_NATIVE_INT, mpi.xnum) ) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_mpi, XNUM_NAME,
                          H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(id_tmp, id_x, 0)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  dims[0] = mpi.ny;
  if (( H5LTmake_dataset(ncid_mpi, YNUM_NAME, 1, dims,
                H5T_NATIVE_INT, mpi.ynum) ) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_mpi, YNUM_NAME,
                          H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(id_tmp, id_y, 0)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  dims[0] = mpi.nx;
  dims[1] = mpi.ny;
  if (( file_dspace = H5Screate_simple(2, dims, NULL) ) < 0) HERR(routineName);
  if (( plist = H5Pcreate(H5P_DATASET_CREATE) ) < 0) HERR(routineName);
  if (( H5Pset_fill_value(plist, H5T_NATIVE_FLOAT, &FILLVALUE) ) < 0)
    HERR(routineName);
  if (( H5Pset_alloc_time(plist, H5D_ALLOC_TIME_EARLY) ) < 0) HERR(routineName);
  if (( H5Pset_fill_time(plist, H5D_FILL_TIME_ALLOC) ) < 0) HERR(routineName);
  if (( io.in_mpi_tm = H5Dcreate(ncid_mpi, TASK_MAP, H5T_NATIVE_LONG,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_tm, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_tm, id_y, 1)) < 0) HERR(routineName);
  if (( io.in_mpi_tn = H5Dcreate(ncid_mpi, TASK_NUMBER, H5T_NATIVE_LONG,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_tn, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_tn, id_y, 1)) < 0) HERR(routineName);
  if (( io.in_mpi_it = H5Dcreate(ncid_mpi, ITER_NAME, H5T_NATIVE_LONG,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_it, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_it, id_y, 1)) < 0) HERR(routineName);
  if (( io.in_mpi_conv = H5Dcreate(ncid_mpi, CONV_NAME, H5T_NATIVE_LONG,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_conv, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_conv, id_y, 1)) < 0) HERR(routineName);
  if (( io.in_mpi_dm = H5Dcreate(ncid_mpi, DM_NAME, H5T_NATIVE_FLOAT,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_dm, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_dm, id_y, 1)) < 0) HERR(routineName);
  if (( io.in_mpi_zc = H5Dcreate(ncid_mpi, ZC_NAME, H5T_NATIVE_INT,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_zc, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_zc, id_y, 1)) < 0) HERR(routineName);
  if (( H5Pclose(plist) ) < 0) HERR(routineName);
  if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);

  dims[0] = mpi.nx;
  dims[1] = mpi.ny;
  dims[2] = NMaxIter;
  if (( file_dspace = H5Screate_simple(3, dims, NULL) ) < 0) HERR(routineName);
  if (( plist = H5Pcreate(H5P_DATASET_CREATE) ) < 0) HERR(routineName);
  if (( H5Pset_fill_value(plist, H5T_NATIVE_FLOAT, &FILLVALUE) ) < 0)
    HERR(routineName);
  if (( H5Pset_alloc_time(plist, H5D_ALLOC_TIME_EARLY) ) < 0) HERR(routineName);
  if (( H5Pset_fill_time(plist, H5D_FILL_TIME_ALLOC) ) < 0) HERR(routineName);
  if (( io.in_mpi_dmh = H5Dcreate(ncid_mpi, DMH_NAME, H5T_NATIVE_FLOAT,
         file_dspace, H5P_DEFAULT, plist, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( id_tmp = H5Dopen2(ncid_mpi, IT_NAME, H5P_DEFAULT)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_dmh, id_x, 0)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_dmh, id_y, 1)) < 0) HERR(routineName);
  if (( H5DSattach_scale(io.in_mpi_dmh, id_tmp, 2)) < 0) HERR(routineName);
  if (( H5Dclose(id_tmp) ) < 0) HERR(routineName);
  if (( H5Pclose(plist) ) < 0) HERR(routineName);
  if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);

  /* attributes */
  if (( H5LTset_attribute_int(ncid_mpi, ".", "x_start",
                              &input.p15d_x0, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_mpi, ".", "x_end",
                              &input.p15d_x1, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_mpi, ".", "x_step",
                              &input.p15d_xst, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_mpi, ".", "y_start",
                              &input.p15d_y0, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_mpi, ".", "y_end",
                              &input.p15d_y1, 1) ) < 0) HERR(routineName);
  if (( H5LTset_attribute_int(ncid_mpi, ".", "y_step",
                              &input.p15d_yst, 1) ) < 0) HERR(routineName);

  /* Tiago: most of the arrays involving Ntasks or rank as index are not
            currently being written. They should eventually be migrated into
            arrays of [ix, iy] and be written for each task. This is to
            avoid causing problems with pool mode, where these quantities are
            not known from the start.
  */
  if (( H5Dclose(id_x) ) < 0) HERR(routineName);
  if (( H5Dclose(id_y) ) < 0) HERR(routineName);
  if (( H5Dclose(id_z) ) < 0) HERR(routineName);
  /* Flush ensures file is created in case of crash */
  if (( H5Fflush(ncid, H5F_SCOPE_LOCAL) ) < 0) HERR(routineName);
  /* --- Copy stuff to the IO data struct --- */
  io.in_ncid       = ncid;
  io.in_input_ncid = ncid_input;
  io.in_atmos_ncid = ncid_atmos;
  io.in_mpi_ncid   = ncid_mpi;
  return;
}
/* ------- end   --------------------------   init_hdf5_indata.c  --- */

/* ------- begin --------------------------   init_hdf5_indata_old.c  --- */
void init_hdf5_indata_existing(void)
/* Opens an existing input data file, loads structures and ids */
{
  const char routineName[] = "init_hdf5_indata_existing";
  int     ncid;
  size_t  attr_size;
  hid_t   plist;
  char   *atmosID;
  H5T_class_t type_class;

  /* Open the file with parallel MPI-IO access */
  if (( plist = H5Pcreate(H5P_FILE_ACCESS )) < 0) HERR(routineName);
  if (( H5Pset_fapl_mpio(plist, mpi.comm, mpi.info) ) < 0) HERR(routineName);
  if (( ncid = H5Fopen(INPUTDATA_FILE, H5F_ACC_RDWR, plist) ) < 0)
    HERR(routineName);
  if (( H5Pclose(plist) ) < 0) HERR(routineName);
  io.in_ncid = ncid;
  /* --- Consistency checks --- */
  /* Check that atmosID is the same */
  if (( H5LTget_attribute_info(ncid, "/", "atmosID", NULL, &type_class,
                               &attr_size) ) < 0) HERR(routineName);
  atmosID = (char *) malloc(attr_size + 1);
  if (( H5LTget_attribute_string(ncid, "/", "atmosID", atmosID) ) < 0)
    HERR(routineName);
  if (!strstr(atmosID, atmos.ID)) {
    sprintf(messageStr,
       "Indata file was calculated for different atmosphere (%s) than current",
	     atmosID);
    Error(WARNING, routineName, messageStr);
    }
  free(atmosID);
  /* Get group IDs */
  if (( io.in_input_ncid = H5Gopen(ncid, "input", H5P_DEFAULT) ) < 0)
      HERR(routineName);
  if (( io.in_atmos_ncid = H5Gopen(ncid, "atmos", H5P_DEFAULT) ) < 0)
      HERR(routineName);
  if (( io.in_mpi_ncid = H5Gopen(ncid, "mpi", H5P_DEFAULT) ) < 0)
      HERR(routineName);
  /* --- Open datasets collectively ---*/
  if (( io.in_atmos_T = H5Dopen(io.in_atmos_ncid, "temperature",
                                H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_atmos_vz = H5Dopen(io.in_atmos_ncid, "velocity_z",
                                H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_atmos_z = H5Dopen(io.in_atmos_ncid, "height_scale",
                                H5P_DEFAULT) ) < 0) HERR(routineName);

  if (( io.in_mpi_tm = H5Dopen(io.in_mpi_ncid, TASK_MAP,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_mpi_tn = H5Dopen(io.in_mpi_ncid, TASK_NUMBER,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_mpi_it = H5Dopen(io.in_mpi_ncid, ITER_NAME,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_mpi_conv = H5Dopen(io.in_mpi_ncid, CONV_NAME,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_mpi_dm = H5Dopen(io.in_mpi_ncid, DM_NAME,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_mpi_dmh = H5Dopen(io.in_mpi_ncid, DMH_NAME,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  if (( io.in_mpi_zc = H5Dopen(io.in_mpi_ncid, ZC_NAME,
                               H5P_DEFAULT) ) < 0) HERR(routineName);
  return;
}
/* ------- end   --------------------------   init_hdf5_indata_old.c  --- */


/* ------- begin --------------------------   close_hdf5_indata.c --- */
void close_hdf5_indata(void)
/* Closes the indata file */
{
  const char routineName[] = "close_hdf5_indata";

  /* Close all datasets */
  if (( H5Dclose(io.in_atmos_T) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_atmos_vz) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_atmos_z) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_tm) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_tn) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_it) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_conv) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_dm) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_dmh) ) < 0) HERR(routineName);
  if (( H5Dclose(io.in_mpi_zc) ) < 0) HERR(routineName);

  /* Close all groups */
  if (( H5Gclose(io.in_input_ncid) ) < 0) HERR(routineName);
  if (( H5Gclose(io.in_atmos_ncid) ) < 0) HERR(routineName);
  if (( H5Gclose(io.in_mpi_ncid) ) < 0) HERR(routineName);

  /* Close file */
  if (( H5Fclose(io.in_ncid) ) < 0) HERR(routineName);
  return;
}
/* ------- end   --------------------------   close_hdf5_indata.c --- */

/* ------- begin --------------------------   writeAtmos_p.c --- */
void writeAtmos_p(void)
{
  /* Write atmos arrays. This has now been modified and writes the interpolated
     arrays, from depth_refine. With that, now this is the only viable option
     to write the atmos data, as there is no option to save in memory and
     writeAtmos_all used to write from the input file, not the interpolated
     quantities

     IMPORTANT: at the moment this is a trimmed version, only writing z to save
                space and computational time.

     */
  const char routineName[] = "writeAtmos_p";
  hsize_t  offset[] = {0, 0, 0, 0};
  hsize_t  count[] = {1, 1, 1, 1};
  hsize_t  dims[4];
  hid_t    file_dspace, mem_dspace;

  /* Memory dataspace */
  dims[0] = atmos.Nspace;
  if (( mem_dspace = H5Screate_simple(1, dims, NULL) ) < 0)
    HERR(routineName);
  /* File dataspace */
  offset[0] = mpi.ix;
  offset[1] = mpi.iy;
  offset[2] = mpi.zcut;
  count[2] = atmos.Nspace;
  if (( file_dspace = H5Dget_space(io.in_atmos_T) ) < 0) HERR(routineName);
  if (( H5Sselect_hyperslab(file_dspace, H5S_SELECT_SET, offset,
                            NULL, count, NULL) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_atmos_T, H5T_NATIVE_DOUBLE, mem_dspace,
                file_dspace, H5P_DEFAULT, atmos.T) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_atmos_vz, H5T_NATIVE_DOUBLE, mem_dspace,
           file_dspace, H5P_DEFAULT, geometry.vel) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_atmos_z, H5T_NATIVE_DOUBLE, mem_dspace,
        file_dspace, H5P_DEFAULT, geometry.height) ) < 0) HERR(routineName);
  /* release dataspace resources */
  if (( H5Sclose(mem_dspace) ) < 0) HERR(routineName);
  if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);
  return;
}
/* ------- end   --------------------------   writeAtmos_p.c --- */

/* ------- begin --------------------------   writeMPI_all.c --- */
void writeMPI_all(void) {
/* Writes output on indata file, MPI group, all tasks at once */
  const char routineName[] = "writeMPI_all";
  int      task;
  hsize_t  offset[] = {0, 0, 0, 0};
  hsize_t  count[] = {1, 1, 1, 1};
  hsize_t  dims[4];
  hid_t    file_dspace, mem_dspace;

  /* Write single values of Ntasks, one value at a time */
  dims[0] = 1;
  if (( mem_dspace = H5Screate_simple(1, dims, NULL) ) < 0)
    HERR(routineName);

  for (task = 0; task < mpi.Ntasks; task++) {
    offset[0] = mpi.taskmap[task + mpi.my_start][0];
    offset[1] = mpi.taskmap[task + mpi.my_start][1];
    if (( file_dspace = H5Dget_space(io.in_mpi_it) ) < 0) HERR(routineName);
    if (( H5Sselect_hyperslab(file_dspace, H5S_SELECT_SET, offset,
                              NULL, count, NULL) ) < 0) HERR(routineName);
    if (( H5Dwrite(io.in_mpi_it, H5T_NATIVE_INT, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.niter[task]) ) < 0) HERR(routineName);
    if (( H5Dwrite(io.in_mpi_conv, H5T_NATIVE_INT, mem_dspace, file_dspace,
                H5P_DEFAULT, &mpi.convergence[task]) ) < 0) HERR(routineName);
    if (( H5Dwrite(io.in_mpi_zc, H5T_NATIVE_INT, mem_dspace, file_dspace,
                  H5P_DEFAULT, &mpi.zcut_hist[task]) ) < 0) HERR(routineName);
    if (( H5Dwrite(io.in_mpi_dm, H5T_NATIVE_DOUBLE, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.dpopsmax[task]) ) < 0) HERR(routineName);
    if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);
  }
  if (( H5Sclose(mem_dspace) ) < 0) HERR(routineName);

  /* Write array with multiple values */
  for (task = 0; task < mpi.Ntasks; task++) {
    dims[0] = mpi.niter[task];
    if (( mem_dspace = H5Screate_simple(1, dims, NULL) ) < 0)
      HERR(routineName);
    offset[0] = mpi.taskmap[task + mpi.my_start][0];
    offset[1] = mpi.taskmap[task + mpi.my_start][1];
    count[2] = mpi.niter[task];
    if (( file_dspace = H5Dget_space(io.in_mpi_dmh) ) < 0) HERR(routineName);
    if (( H5Sselect_hyperslab(file_dspace, H5S_SELECT_SET, offset,
                              NULL, count, NULL) ) < 0) HERR(routineName);
    if (( H5Dwrite(io.in_mpi_dmh, H5T_NATIVE_DOUBLE, mem_dspace, file_dspace,
               H5P_DEFAULT, mpi.dpopsmax_hist[task]) ) < 0) HERR(routineName);
    if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);
    if (( H5Sclose(mem_dspace) ) < 0) HERR(routineName);
  }
  return;
}
/* ------- end   --------------------------   writeMPI_all.c --- */


/* ------- begin --------------------------   writeMPI_p.c ----- */
void writeMPI_p(int task) {
/* Writes output on indata file, MPI group, one task at once */
  const char routineName[] = "writeMPI_p";
  hsize_t  offset[] = {0, 0, 0, 0};
  hsize_t  count[] = {1, 1, 1, 1};
  hsize_t  dims[4];
  hid_t    file_dspace, mem_dspace;

  dims[0] = 1;
  if (( mem_dspace = H5Screate_simple(1, dims, NULL) ) < 0) HERR(routineName);
  offset[0] = mpi.ix;
  offset[1] = mpi.iy;
  if (( file_dspace = H5Dget_space(io.in_mpi_tm) ) < 0) HERR(routineName);
  if (( H5Sselect_hyperslab(file_dspace, H5S_SELECT_SET, offset,
                            NULL, count, NULL) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_tm, H5T_NATIVE_INT, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.rank) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_tn, H5T_NATIVE_INT, mem_dspace, file_dspace,
                   H5P_DEFAULT, &task) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_it, H5T_NATIVE_INT, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.niter[0]) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_conv, H5T_NATIVE_INT, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.convergence[0]) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_zc, H5T_NATIVE_INT, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.zcut_hist[0]) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_dm, H5T_NATIVE_DOUBLE, mem_dspace, file_dspace,
                   H5P_DEFAULT, &mpi.dpopsmax[0]) ) < 0) HERR(routineName);
  if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);
  if (( H5Sclose(mem_dspace) ) < 0) HERR(routineName);

  dims[0] = mpi.niter[0];
  if (( mem_dspace = H5Screate_simple(1, dims, NULL) ) < 0) HERR(routineName);
  offset[0] = mpi.ix;
  offset[1] = mpi.iy;
  count[2] = mpi.niter[0];
  if (( file_dspace = H5Dget_space(io.in_mpi_dmh) ) < 0) HERR(routineName);
  if (( H5Sselect_hyperslab(file_dspace, H5S_SELECT_SET, offset,
                            NULL, count, NULL) ) < 0) HERR(routineName);
  if (( H5Dwrite(io.in_mpi_dmh, H5T_NATIVE_DOUBLE, mem_dspace, file_dspace,
                 H5P_DEFAULT, mpi.dpopsmax_hist[0]) ) < 0) HERR(routineName);
  if (( H5Sclose(file_dspace) ) < 0) HERR(routineName);
  if (( H5Sclose(mem_dspace) ) < 0) HERR(routineName);
  return;
}
/* ------- end   --------------------------   writeMPI_p.c ------- */

/* ------- begin -------------------------- readConvergence.c  --- */
void readConvergence(void) {
  /* This is a self-contained function to read the convergence matrix,
     written by RH. */
  const char routineName[] = "readConvergence";
  char *atmosID;
  int ncid, ncid_mpi, nx, ny;
  size_t attr_size;
  hid_t plist;
  H5T_class_t type_class;

  mpi.rh_converged = matrix_int(mpi.nx, mpi.ny);

  /* --- Open the inputdata file --- */
  if (( plist = H5Pcreate(H5P_FILE_ACCESS )) < 0) HERR(routineName);
  if (( H5Pset_fapl_mpio(plist, mpi.comm, mpi.info) ) < 0) HERR(routineName);
  if (( ncid = H5Fopen(INPUTDATA_FILE, H5F_ACC_RDWR, plist) ) < 0)
    HERR(routineName);
  if (( H5Pclose(plist) ) < 0) HERR(routineName);
  /* Get ncid of the MPI group */
  if (( ncid_mpi = H5Gopen(ncid, "mpi", H5P_DEFAULT) ) < 0) HERR(routineName);

  /* --- Consistency checks --- */
  /* Check that atmosID is the same */
  if (( H5LTget_attribute_info(ncid, "/", "atmosID", NULL, &type_class,
                               &attr_size) ) < 0) HERR(routineName);
  atmosID = (char *) malloc(attr_size + 1);
  if (( H5LTget_attribute_string(ncid, "/", "atmosID", atmosID) ) < 0)
    HERR(routineName);
  if (!strstr(atmosID, atmos.ID)) {
    sprintf(messageStr,
       "Indata file was calculated for different atmosphere (%s) than current",
	     atmosID);
    Error(WARNING, routineName, messageStr);
    }
  free(atmosID);
  /* Check that dimension sizes match */
  if (( H5LTget_attribute_int(ncid, "/", "nx", &nx) ) < 0) HERR(routineName);
  if (nx != mpi.nx) {
    sprintf(messageStr,
	    "Number of x points mismatch: expected %d, found %d.",
	    mpi.nx, (int)nx);
    Error(WARNING, routineName, messageStr);
  }
  if (( H5LTget_attribute_int(ncid, "/", "ny", &ny) ) < 0) HERR(routineName);
  if (ny != mpi.ny) {
    sprintf(messageStr,
	    "Number of y points mismatch: expected %d, found %d.",
	    mpi.ny, (int)ny);
    Error(WARNING, routineName, messageStr);
  }
  /* --- Read variable --- */
  if (( H5LTread_dataset_int(ncid_mpi, CONV_NAME,
                             mpi.rh_converged[0]) ) < 0) HERR(routineName);
  /* --- Close inputdata file --- */
  if (( H5Gclose(ncid_mpi) ) < 0) HERR(routineName);
  if (( H5Fclose(ncid) ) < 0) HERR(routineName);
  return;
}
/* ------- end   -------------------------- readConvergence.c  --- */
