import os
import sys
import optparse

parser = optparse.OptionParser()

parser.add_option("--qchome", dest="qchome", help="Location of the QC Tools packages (mandatory)")
parser.add_option("--r_list", dest="r_list", help="List containing image files for r band")
parser.add_option("--g_list", dest="g_list", help="List containing image files for g band")
parser.add_option("--i_list", dest="i_list", help="List containing image files for i band")
parser.add_option("--z_list", dest="z_list", help="List containing image files for z band")
parser.add_option("--base", dest="base", help="Set runBaseDir explicitly")

(options, args) = parser.parse_args()

if options.base:
	runBaseDir = options.base
else:
	runBaseDir = os.getcwd()


if not options.qchome:
	print "Wrong parameters received. Please check the command line arguments."
	print "Use --help for additional information"
	sys.exit(1)

sys.path.insert(0,options.qchome+'/pypeline/')

#os.environ['PATH'] = options.qchome+'/bin:'+os.environ['PATH']
sys.path.insert(0,options.qchome+'/lib64/python/')
sys.path.insert(0,options.qchome+'/lib64/python/setuptools-0.6c6-py2.3.egg')

try:
	import pipeline
except:
	print 'Could not load the pypeline modules.'
	print 'Please check your --qchome and try again.'
	sys.exit(2)



import Ft.Xml.Domlette as dom

#Replacing template values with those provided in the command line


files_list = []

for f in [options.r_list, options.g_list, options.i_list, options.z_list]:
	if f and len(f) > 0:
		try:
			files_list += open(runBaseDir+"/log/"+f).readlines()
		except:
			print "file: "+runBaseDir+"/log/"+f+" was not found"
			print "Check you --base parameter or you current working directory" 


for i in files_list:
	i = i.strip('\n')
	img_file = i.split('/')[-1]
	baseName = img_file.rstrip(".fits")
	catalogFile = baseName+"_cat.fits"
	img_file = baseName+"_im.fits"
	movingPath = "/".join(i.split('/')[:-1])
	pype = pipeline.PipeLine(uniq=baseName)
	pype.loadConfig(options.qchome+'/etc/qc.xml')
	(pype.doc.xpath('/job/config/rootDir')[0]).firstChild.nodeValue = runBaseDir+"/log/qc"
	(pype.doc.xpath('/job/config/dataRepository')[0]).firstChild.nodeValue = runBaseDir+"/"+movingPath
	(pype.doc.xpath('/job/components/component[@id="origin"]/data/file[@id="fits_me_image"]')[0]).setAttributeNS(None, u'value', img_file)
	(pype.doc.xpath('/job/components/component[@id="origin"]/data/file[@id="ldac_catalog"]')[0]).setAttributeNS(None, u'value', catalogFile)
#	pype.printConfig()
	pype.execute()
	
