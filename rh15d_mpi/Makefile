## ------- file: ------------------------- Makefile ----------------- ##
#
#      Version:       rh2.0, 1.5-D plane-parallel
#      Author:        Tiago Pereira (tiago.pereira@nasa.gov)
#      Last modified: Wed Nov 24 10:23:39 2010 --
#
##     --------------------------                      ----------RH-- ##

GEOMETRY = ONE_D_PLANE

include ../makefiles/makefile.$(CPU).$(OS)

CC = mpicc
LD = mpicc
AR = ar
HDF5_DIR = /opt/local

# Try to get git revision, set preprocessor macros if successful
REV=$(shell git describe --always)
ifeq ($(REV), )
    CFLAGS  = -O0 -g -I../ -I$(HDF5_DIR)/include/
else
    REV = $(shell git log -1 --pretty=format:"%h  %an  %ai")
    CFLAGS  = -O2 -I../ -I$(HDF5_DIR)/include/ -D'REV_ID="$(REV)"'
endif

LDFLAGS = -L../ -L$(HDF5_DIR)/lib/


## --- If no FORTRAN compiler is available remove -lrh_f90 in following

LIBS = -lrh -lrh_f90 $(ARCHLIBS) -lhdf5 -lhdf5_hl


## --- Define groups of object files --                -------------- ##

ONE_D_OBJS = anglequad.o      feautrier.o     formal.o           hydrostat.o   \
             piecestokes.o    piecewise.o     project.o          hdf5atmos.o   \
             riiplane.o       background_p.o  distribute_jobs.o  escape.o      \
             scatter_p.o      initial_p.o     bezier.o           writeAux_p.o  \
             writeindata_p.o  parallel.o      iterate_p.o        ludcmp_p.o    \
             statequil_p.o    accelerate_p.o  redistribute_p.o   multiatmos.o  \
			 readatmos.o

SUBSTITUTE = pops_xdr.o

   ONEDOBJ = iter_1d.o        feautrier.o


## --- Rules for the executables --                    -------------- ##

all:    rh15d_ray rh15d_ray_pool rh15d_lteray

rh15d_lteray:  $(ONE_D_OBJS)  librh rh15d_lteray.o writeRay.o
	$(LD) -o $@  $(LDFLAGS) $(ONE_D_OBJS) rh15d_lteray.o writeRay.o $(LIBS)

rh15d_ray:  $(ONE_D_OBJS)  librh rh15d_ray.o writeRay.o
	$(LD) -o $@  $(LDFLAGS) $(ONE_D_OBJS) rh15d_ray.o writeRay.o $(LIBS)

rh15d_ray_pool:  $(ONE_D_OBJS)  librh rh15d_ray_pool.o writeRay.o
	$(LD) -o $@  $(LDFLAGS) $(ONE_D_OBJS) rh15d_ray_pool.o writeRay.o $(LIBS)


## --- If no FORTRAN compiler is available remove librh_f90.a in following

librh:
	cd ..; $(MAKE) librh.a  librh_f90.a


## --- Clean up --                                     -------------- ##

clean:
	rm -f *.o rh15d_ray rh15d_ray_pool rh15d_lteray


## --- Explicit dependencies on include files --       -------------- ##

accelerate_p.o:         ../rh.h          ../accelerate.h  ../error.h       \
                        ../statistics.h  parallel.h

anglequad.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h

backgrcontr.o:          ../rh.h          ../atom.h        ../atmos.h       \
                        ../spectrum.h    geometry.h       ../background.h  \
                        ../inputs.h      ../error.h       ../statistics.h

background_p.o:         ../rh.h          ../atom.h        ../atmos.h       \
                         ../spectrum.h   ../constant.h    ../background.h  \
                        ../error.h       ../statistics.h  ../inputs.h      \
                        geometry.h       parallel.h       io.h             \
                        background_p.c
	$(CC) $(CFLAGS) -Wall -c  -o $@  background_p.c

brs_p.o:                ../rh.h          ../atom.h        ../atmos.h       \
                        ../spectrum.h    ../error.h       geometry.h       \
                        parallel.h       io.h             brs_p.c
	$(CC) $(CFLAGS) -Wall -c  -o $@  brs_p.c

distribute_jobs.o:      ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../inputs.h      ../error.h       \
                        parallel.h       io.h             distribute_jobs.c
	$(CC) $(CFLAGS) -Wall -c  -o $@  distribute_jobs.c

escape.o:               ../rh.h          ../atom.h        ../atmos.h       \
                        ../background.h  geometry.h       ../spectrum.h    \
			../error.h       ../inputs.h      ../constant.h    \
			parallel.h       escape.c
	$(CC) $(CFLAGS) -Wall -c  -o $@  escape.c

expint_p.o:             ../rh.h          ../error.h       parallel.h

feautrier.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../spectrum.h    ../error.h

formal.o:               ../rh.h          ../atom.h        ../atmos.h       \
                        parallel.h       geometry.h       ../spectrum.h    \
                        ../constant.h    ../background.h  ../inputs.h      \
			../error.h

hydrostat.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../background.h  ../constant.h    \
                        ../accelerate.h  ../error.h

initial_p.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        ../spectrum.h    ../accelerate.h  ../constant.h    \
                        ../statistics.h  ../error.h       ../inputs.h      \
                        parallel.h       initial_p.c
	$(CC) $(CFLAGS) -Wall -c  -o $@  initial_p.c

iter_1d.o:              ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../accelerate.h  ../error.h       \
                        ../statistics.h


iterate_p.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        ../spectrum.h    ../background.h  ../accelerate.h  \
                        ../error.h       ../statistics.h  ../inputs.h      \
                        parallel.h       iterate_p.c
	$(CC) $(CFLAGS) -Wall -c -o $@   iterate_p.c

ludcmp_p.o:             ../rh.h          ../error.h       parallel.h       \
                        ludcmp_p.c
	$(CC) $(CFLAGS) -Wall -c -o $@   ludcmp_p.c

parallel.o:             ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../error.h       ../inputs.h      \
                        ../constant.h    ../accelerate.h  parallel.h       \
	                io.h             parallel.c
	$(CC) $(CFLAGS) -Wall -c -o $@   parallel.c

piecestokes.o:          ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../spectrum.h

bezier.o:               ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../spectrum.h    bezier.h

piecewise.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../spectrum.h

project.o:              ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h


readj_p.o:              ../rh.h          ../atom.h        ../atmos.h       \
                        ../spectrum.h    parallel.h       ../background.h  \
                        geometry.h       ../inputs.h      ../error.h       \
	                io.h             readj_p.c
	$(CC) $(CFLAGS) -Wall -c  -o $@  readj_p.c

redistribute_p.o:       ../rh.h          ../atmos.h       ../atom.h        \
                        ../spectrum.h    ../accelerate.h  ../error.h       \
                        ../inputs.h      parallel.h

rh15d_lteray.o:         ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../spectrum.h    ../background.h  \
                        ../statistics.h  ../error.h       ../inputs.h      \
                        parallel.h       io.h             rh15d_lteray.c
	$(CC) $(CFLAGS) -Wall -c -o $@   rh15d_lteray.c

rh15d_ray.o:            ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../spectrum.h    ../background.h  \
                        ../statistics.h  ../error.h       ../inputs.h      \
                        parallel.h       io.h             rh15d_ray.c
	$(CC) $(CFLAGS) -Wall -c -o $@   rh15d_ray.c

rh15d_ray_pool.o:            ../rh.h          ../atom.h        ../atmos.h  \
                        geometry.h       ../spectrum.h    ../background.h  \
                        ../statistics.h  ../error.h       ../inputs.h      \
                        parallel.h       io.h             rh15d_ray_pool.c
	$(CC) $(CFLAGS) -Wall -c -o $@   rh15d_ray_pool.c

riiplane.o:             ../rh.h          ../atom.h        ../atmos.h       \
                        geometry.h       ../constant.h    ../error.h

scatter_p.o:            ../rh.h          ../atom.h        ../atmos.h      \
                        ../spectrum.h    ../inputs.h      ../constant.h   \
                        ../error.h       ../statistics.h

statequil_p.o:          ../rh.h          ../atom.h        ../atmos.h      \
                        ../accelerate.h  ../error.h       ../inputs.h     \
                        parallel.h


writeAux_p.o:           ../rh.h          ../atom.h        ../atmos.h      \
                        geometry.h       ../spectrum.h    ../error.h      \
                        ../inputs.h      parallel.h       io.h            \
                        writeAux_p.c
	$(CC) $(CFLAGS) -Wall -c -o $@   writeAux_p.c

writeindata_p.o:       ../rh.h           ../atom.h        ../atmos.h      \
                        geometry.h       ../error.h       ../inputs.h     \
                        parallel.h       io.h             writeindata_p.c
	$(CC) $(CFLAGS) -Wall -c -o $@   writeindata_p.c

writeRay.o:             ../rh.h          ../atom.h        ../atmos.h      \
	                ../spectrum.h    ../constant.h    ../background.h \
                        ../error.h       ../inputs.h      parallel.h      \
                        io.h             writeRay.c
	$(CC) $(CFLAGS) -Wall -c -o $@   writeRay.c

## ------- end ---------------------------- Makefile ---------------- ##
