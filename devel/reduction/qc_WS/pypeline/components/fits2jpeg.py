import Ft.Xml.Domlette as dom
import Ft.Xml as xml
from Ft.Lib import Uri
import os

import pipeline.io as cpio

def run():
	conf = cpio.ComponentConfig()
	io = cpio.ComponentIO()
	f_input = io.getFirstFile()
	f_output = f_input.rstrip('fits')+"jpg"
	min_image_value = conf.getScalarById('min_image_value')
	max_image_value = conf.getScalarById('max_image_value')
	command_line = 'fits2jpeg -fits %s -jpeg %s -min %s -max %s' % (f_input, f_output, min_image_value, max_image_value)
	#command_line = 'fits2jpeg -fits %s -jpeg %s' % (f_input, f_output,)
	res = os.spawnvp(os.P_WAIT, 'fits2jpeg', command_line.split())
	if (res):
		raise Exception, 'Could not run command: '+command_line
		exit()

	io.addOutput(f_output,"jpeg_file","jpeg_image",'Field Image','Field-of-view image',"file")

