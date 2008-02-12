"""
This is a module to match a catalog extracted from an image chosen by QC Prototype with USNOB1 reference catalog.
Created by Bruno Rossetto.
"""

###########
# Imports #
###########

try:
        import sys
        import os
        sys.path.append('/home/rossetto/devel/qctools/qctools/static/cgi-bin/')
        import matchFactory1
        import plotFactory1
        import pyfits
        from numpy import min, max, sqrt, array
        import time
        import pipeline.io as cpio
	import pipeline.etc

except ImportError, err:
        print err
        exit()


def run():

        conf = cpio.ComponentConfig()
        io = cpio.ComponentIO()


        catFile = io.getFirstFile() #Input catalog. Must be given.
        refFile = io.getFileById("ref_file") #Reference Catalog file.
        E = conf.getScalarById('ang_sep') #Angular separation in arcsec. Must be given.
        outFile = "match.out" #Outputname of match file.
	filter = conf.getScalarById('band')
        sig = conf.getScalarById('sigma_cut') #Angular separation in arcsec. Must be given.

        if sig == 3.0:
                s = '3'
        if sig == 5.0:
                s = '5'
        if sig == 10.0:
                s = '10'

        if filter == "U":
                band = "_01"
		band_ref = "_00"
        elif filter == "V":
                band = "_02"
		band_ref = "_02"
        elif filter == "I":
                band = "_03"
		band_ref = "_04"
	elif filter == "J":
		band = "_04"
		band_ref = "_05"
	elif filter == "K":
		band = "_05"
		band_ref = "_06"
        else:
                return False


        X = pyfits.open(catFile)
        Y = pyfits.open(refFile)

        bol_flagtrim = (X[pipeline.etc.LDACObjectsTable].data.field('flag_trim'+band) == 0).tolist()
        bol_flagmask = (X[pipeline.etc.LDACObjectsTable].data.field('flag_mask'+band) == 0).tolist()
        bol_flagstate = (X[pipeline.etc.LDACObjectsTable].data.field('flag_state'+band) == 0).tolist()
#        bol_flagsigma = (X[pipeline.etc.LDACObjectsTable].data.field('flag_'+s+'sigma') == 0).tolist()

        util = []
        for i in range(len(bol_flagtrim)):
                a = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i]
                util.append(a)

        ra_In = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('alpha_j2000'+band).tolist()
        dec_In = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('delta_j2000'+band).tolist()
        mag_In =  X[pipeline.etc.LDACObjectsTable].data[array(util)].field('mag_auto'+band).tolist()
        mag_err = X[pipeline.etc.LDACObjectsTable].data[array(util)].field('magerr_auto'+band).tolist()

        bol_flagtrim = (Y[1].data.field('flag_trim'+band_ref) == 0).tolist()
        bol_flagmask = (Y[1].data.field('flag_mask'+band_ref) == 0).tolist()
        bol_flagstate = (Y[1].data.field('flag_state'+band_ref) == 0).tolist()
#        bol_flagsigma = (X[pipeline.etc.LDACObjectsTable].data.field('flag_'+s+'sigma') == 0).tolist()

        util = []
        for i in range(len(bol_flagtrim)):
                a = bol_flagtrim[i] and bol_flagmask[i] and bol_flagstate[i]
                util.append(a)

        ra_Ref = Y[1].data[array(util)].field('alpha_j2000'+band_ref).tolist()
        dec_Ref = Y[1].data[array(util)].field('delta_j2000'+band_ref).tolist()
        mag_Ref = Y[1].data[array(util)].field('mag_auto'+band_ref).tolist()

	raIn = []
	decIn = []
	magIn = []
	magerr = []
	for i in range(len(ra_In)):
		if ra_In[i] != 0.0 and dec_In[i] != 0.0:
			raIn.append(ra_In[i])
			decIn.append(dec_In[i])
			magIn.append(mag_In[i])
			magerr.append(mag_err[i])

	raRef = []
	decRef = []
	magRef = []
	for i in range(len(ra_Ref)):
                if ra_Ref[i] != 0.0 and dec_Ref[i] != 0.0:
                        raRef.append(ra_Ref[i])
                        decRef.append(dec_Ref[i])
			magRef.append(mag_Ref[i])

        matchFactory1.crossMatch(outFile,raIn,decIn,magIn,raRef,decRef,magRef,E)

        X.close()
        Y.close()


        X = open(outFile).read().splitlines()

        dra = []
        ddec = []
        dpos = []
        ra = []
        dec = []
        dmag = []
        mag1 = []
        mag2 = []
        n = []

        for i in range(len(X)-1):
                n.append(int(X[i].split()[7]))
                if int(X[i].split()[7]) == 1 and int(X[i+1].split()[7]) == 1:
                        ra.append(float(X[i].split()[0]))
                        dec.append(float(X[i].split()[1]))
                        dra.append(float(X[i].split()[2]))
                        ddec.append(float(X[i].split()[3]))
                        dpos.append(sqrt(float(X[i].split()[2])**2 + float(X[i].split()[3])**2))
                        mag1.append(float(X[i].split()[4]))
                        mag2.append(float(X[i].split()[5]))
                        dmag.append(float(X[i].split()[6]))

	n.append(int(X[len(X)-1].split()[7]))
        if X[len(X)-1].split()[7] == 1:
                ra.append(float(X[len(X)-1].split()[0]))
                dec.append(float(X[len(X)-1].split()[1]))
                dra.append(float(X[len(X)-1].split()[2]))
                ddec.append(float(X[len(X)-1].split()[3]))
                dpos.append(sqrt(float(X[len(X)-1].split()[2])**2 + float(X[len(X)-1].split()[3])**2))
                mag1.append(float(X[len(X)-1].split()[4]))
                mag2.append(float(X[len(X)-1].split()[5]))
                dmag.append(float(X[len(X)-1].split()[6]))

	plotName = catFile.rstrip(".fits")+'spatial_delta.png'
        plotName1 = catFile.rstrip(".fits")+'multi_delta.png'
        plotName2 = catFile.rstrip(".fits")+'delta.png'
        plotName3 = catFile.rstrip(".fits")+'delta_ra.png'
        plotName4 = catFile.rstrip(".fits")+'delta_dec.png'

        plotName5 = catFile.rstrip(".fits")+'mag_error.png'
        plotName6 = catFile.rstrip(".fits")+'delta_mag.png'
        plotName7 = catFile.rstrip(".fits")+'delta_mag_mean.png'
        plotName8 = catFile.rstrip(".fits")+'delta_mag_rms.png'

        plotFactory1.plotDeltaMag(plotName6,mag1,dmag)
        plotFactory1.plotMeanDeltaMag(plotName7,mag1,dmag)
        plotFactory1.plotRmsDeltaMag(plotName8,mag1,dmag)
        plotFactory1.plotErrorMag(plotName5,magIn,magerr)

        plotFactory1.plotSpatialDelta(plotName,ra,dec,ramin,ramax,decmin,decmax)
        plotFactory1.plotMultDelta(plotName1,n)
        plotFactory1.plotDelta(plotName2,dra,ddec)
        plotFactory1.plotDeltaRA(plotName3,ra,dpos)
        plotFactory1.plotDeltaDEC(plotName4,dec,dpos)

        os.popen('rm -rf '+outFile)


