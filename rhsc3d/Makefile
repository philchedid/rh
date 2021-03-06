## ------- file: ------------------------- Makefile ----------------- ##
#
#      Version:       rh2.0, 3-D Cartesian, short characteristics
#      Author:        Han Uitenbroek (huitenbroek@nso.edu)
#      Last modified: Wed Apr 22 09:38:22 2009 --
#
##     --------------------------                      ----------RH-- ##


GEOMETRY = THREE_D_PLANE

include ../makefiles/makefile.$(CPU).$(OS)


## --- If no FORTRAN compiler is available remove -lrh_f90 in following

    LIBS = -lrh -lrh_f90 $(ARCHLIBS)


## --- Define groups of object files --                -------------- ##


THREE_D_OBJS = anglequad.o  formal.o  geometry.o  readatmos_xdr.o  rhsc3d.o \
               shortchar.o  solvelong.o  project.o  rii.o  writeflux_xdr.o \
               writegeom_xdr.o

RAY_OBJS = anglequad.o  formal.o  geometry.o \
           readatmos_xdr.o  rii.o  shortchar.o  solvelong.o \
           solveray.o  project.o

FORMOBJS = anglequad.o  formal3d.o  geometry.o \
           shortchar.o  solvelong.o  project.o



## --- Rules for the executables --                    -------------- ##

all:   rhsc3d  solveray  formal3d

rhsc3d:  $(THREE_D_OBJS)  librh
	$(LD) -o $@  $(LDFLAGS) $(THREE_D_OBJS) $(LIBS)

solveray:  $(RAY_OBJS)  librh
	$(LD) -o $@  $(LDFLAGS) $(RAY_OBJS) $(LIBS)

formal3d:  $(FORMOBJS)  librh
	$(LD) -o $@  $(LDFLAGS) $(FORMOBJS)  $(LIBS)


## --- If no FORTRAN compiler is available remove librh_f90.a in following

librh:
	cd ..; $(MAKE) librh.a  librh_f90.a


## --- Clean up --                                     -------------- ##

clean:
	rm -f *.o  rhsc3d  solveray  formal3d


## --- Explicit dependencies on include files --       -------------- ##


anglequad.o:            ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../error.h  ../constant.h

formal.o:               ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../spectrum.h  ../error.h  \
                        ../background.h  ../inputs.h  ../constant.h  \
                        ../statistics.h  ../xdr.h

geometry.o:             ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../inputs.h  ../statistics.h  ../error.h

project.o:              ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h

readatmos_xdr.o:        ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../spectrum.h  ../background.h  \
                        ../constant.h  ../statistics.h  ../error.h  \
                        ../inputs.h  ../xdr.h

rhsc3d.o:               ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../spectrum.h  ../background.h  \
                        ../statistics.h  ../inputs.h  ../xdr.h

rii.o:                  ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../constant.h  ../error.h

shortchar.o:            ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../spectrum.h  ../inputs.h  ../error.h

solvelong.o:            ../rh.h  ../atom.h  ../atmos.h  ../spectrum.h \
                        geometry.h

solveray.o:             ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../spectrum.h  ../background.h  \
                        ../statistics.h  ../inputs.h  ../error.h  ../xdr.h

writeflux_xdr.o:        ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../spectrum.h  ../constant.h  \
                        ../error.h  ../xdr.h

writegeom_xdr.o:        ../rh.h  ../atom.h  ../atmos.h  \
                        geometry.h  ../error.h  ../inputs.h  ../xdr.h

## ------- end ---------------------------- Makefile ---------------- ##
