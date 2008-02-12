import Ft.Xml.Domlette as dom
import Ft.Xml as xml
from Ft.Lib import Uri
import os
import shutil #Para copiar os arquivos(temp)
import time
import pipeline.io as cpio
import pipeline.etc

def run():
	conf = cpio.ComponentConfig()
	io = cpio.ComponentIO()
	f_input = io.getFirstFile()
	f_output = f_input.rstrip('fits')+"cat.fits"
	_GAIN = io.getConfigById('gain')
	_PIXEL_SCALE = io.getConfigById('pixel_scale')
	_MAG_ZEROPOINT = io.getConfigById('zero_point')
	_SEEING = io.getConfigById("seeing")
	_WEIGHT_IMAGE = io.getFileById('test.weight')
	#temporario!
	shutil.copy(pipeline.etc.ConfigPath+"sex.config",os.getcwd())

	command_line = 'sex -c SExConfigAttributes.sex %s -CATALOG_NAME %s ' % (f_input, f_output)
	command_line += '-GAIN %s ' % (_GAIN)
	command_line += '-PIXEL_SCALE %s ' % (_PIXEL_SCALE)
	command_line += '-MAG_ZEROPOINT %s ' % (_MAG_ZEROPOINT)
	command_line += '-SEEING_FWHM %s ' % (_SEEING)
	command_line += '-WEIGHT_IMAGE %s ' % (_WEIGHT_IMAGE)
	command_line += '-CATALOG_TYPE FITS_LDAC '

	res = os.spawnvp(os.P_WAIT, 'sex', command_line.split())
	if (res):
		raise Exception, 'Could not run command: '+command_line
		exit()

	io.addOutput(f_output,"ldac_catalog",'fits_catalog',"file")

