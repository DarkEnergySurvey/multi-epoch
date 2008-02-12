default: all

# This also installs by default
all:
	cd cfitsio; ${MAKE}
	cd errors; ${MAKE}
	cd tmv0.54; ${MAKE} small_lib
	cd ConfigFile; ${MAKE}
	cd findstars; ${MAKE}
	cd roundstars; ${MAKE}
	cd measure_stars; ${MAKE}
	cd python; ${MAKE}
	cd measure_stars; ${MAKE} install
	cd python; ${MAKE} install

clean:
	cd cfitsio; ${MAKE} clean
	cd errors; ${MAKE} clean
	cd tmv0.54; ${MAKE} clean
	cd ConfigFile; ${MAKE} clean
	cd findstars; ${MAKE} clean
	cd roundstars; ${MAKE} clean
	cd measure_stars; ${MAKE} clean
	cd python; ${MAKE} clean

install:
	cd measure_stars; ${MAKE} install
	cd python; ${MAKE} install
