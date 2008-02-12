# Library for accessing configuration files and parameters

import Ft.Xml.Domlette as dom
import Ft.Xml as xml
from Ft.Lib import Uri
import sys
import os
import pipeline.system.xmlqc as sxml


class ComponentIO:

	def __init__(self):
		file_uri_in = Uri.OsPathToUri('input.xml')

		self.out_doc = file('output.xml', 'w')
		self.writer = xml.MarkupWriter(self.out_doc, indent=u"yes")
		self.writer.startDocument()
		self.writer.simpleElement(u'data')
		self.writer.endDocument()
		self.out_doc.close()
		try:
			self.doc_in = dom.NonvalidatingReader.parseUri(file_uri_in)
		except:
			raise Exception, "input.xml not found in: '"+os.getcwd()+"'."


	def getFirstFile(self):
		return self.doc_in.xpath('/input/file')[0].attributes[(None,'value')].nodeValue

	def getFileById(self, file_id):
		return self.doc_in.xpath('/input/file[@id="'+file_id+'"]')[0].attributes[(None,'value')].nodeValue

	def getConfigById(self, config_id):
		try: 
			val = self.doc_in.xpath('/input/config[@id="'+config_id+'"]')[0].attributes[(None,'value')].nodeValue
			type = self.doc_in.xpath('/input/config[@id="'+config_id+'"]')[0].attributes[(None,'type')].nodeValue
			if type == 'float':
				return float(val)
			elif type == 'int':
				return int(val)
			elif type == 'string':
				return str(val)
		except:
			return None

	def addOutput(self, f_file, f_id, f_type, f_name, f_sub, output_type):
#		el = self.doc_out.createElementNS(xml.EMPTY_NAMESPACE, 'file')
#		el.appendChild(self.doc_out.createTextNode(f_name))
#		a = self.doc_out.xpath('//output')[0]
#		a.appendChild(el)
#		self.writer.startElement(u'file')
#		self.writer.text(f_name)
#		self.writer.endElement(u'file')
#		self.writer.endElement(u'output')
		dc = sxml.loadXML('output.xml')#, 'bogus') #bogus ??
		el = dc.createElementNS(xml.EMPTY_NAMESPACE, output_type)
		el.setAttributeNS(None, u'type', f_type)
		el.setAttributeNS(None, u'value', f_file)
		el.setAttributeNS(None, u'id', f_id)
		el.setAttributeNS(None, u'sub', f_sub)
		el.setAttributeNS(None, u'name', f_name)
		a = dc.xpath('//data')[0]
		a.appendChild(el)
		f = open('output.xml','w')
		f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
		dom.PrettyPrint(a, stream=f)
		f.close()


#===============================================================
# Configuration functions
#===============================================================
# Library for accessing configuration files and parameters

class ComponentConfig:

	def __init__(self):
		file_uri = Uri.OsPathToUri('config.xml')
		try:
			self.doc = dom.NonvalidatingReader.parseUri(file_uri)
		except:
			import os
			print >>sys.stderr, "ERR: Config file (config.xml) not found in '"+os.getcwd()+"'."

	
	def getScalarById(self, id):
		type = None
		try:
			obj = self.doc.xpath('//scalar[@id="'+id+'"]')

			if ( len(obj) == 0 ):
				raise Exception, "Scalar with id='"+id+"' not present in config.xml"

			if ( len(obj) > 1 ):
				raise Exception, "Scalar with id='"+id+"' not unique in config.xml"

			obj = self.doc.xpath('//scalar[@id="'+id+'"]')[0].attributes
			type = obj.get( (None, 'type') ).nodeValue
			val = obj.get( (None, 'value') ).nodeValue
			if type == 'float':
				return float(val)
			elif type == 'int':
				return int(val)
			elif type == 'bool':
				return bool(val)
			elif type == 'string':
				return str(val)

		except ValueError, (e):
			print >>sys.stderr, 'WARNING: attribute id="'+id+'" - '+e.message
			return 
