#Ft libraries are provided by 4Suite
import Ft.Xml.Domlette as dom
import Ft.Xml as xml
from Ft.Lib import Uri
import time
import os
import sys
import re



class PipeLine:

	def __init__(self, configFile=None, uniq=None):
		if (configFile):
			self.loadConfig(configFile)
		else:
			self.doc = None
		#define ID epoch para ser usada como uma ID na rodada
		if uniq:
			self.uniqId = uniq
		else:
			self.uniqId = str(int(time.time()*1000000))

		
	def loadXML(self, file):
		"""Abre um arquivo(xml) removendo as quebras de linhas e tabs formando um xml linear e retornando um xml aberto pelo Ft"""

		file_string = ''.join(open(file).readlines()).replace('\n','').replace('\t','')
		s1 = re.sub('[\t\n]', '', file_string)
		s2 = re.sub('\ +', ' ', s1)
		s3 = re.sub('> ', '>', s2)
		file_string = re.sub(' <', '<', s3)
		return  dom.NonvalidatingReader.parseString(file_string, self.uniqId)


	def loadConfig(self, configFile):
		"""Define para a class a variavel doc como retorno do loadXML"""

		self.doc = self.loadXML(configFile)

	def printConfig(self):
		dom.PrettyPrint(self.doc)

	def execute(self):
		"""executa"""

		#Define o working dir da rodada e o diretorio do respositorio a ser usado
		rootDir = self.doc.xpath('/job/config/rootDir/text()')[0].nodeValue
		self.runBaseDir = rootDir+'/base-'+self.uniqId
		dataRepository = self.doc.xpath('/job/config/dataRepository/text()')[0].nodeValue

		#Cria os diretorios basicos para a rodada e entra no diretorio runBaseDir
		try:
			os.mkdir(rootDir)
		except:
			pass

		try:
			os.mkdir(self.runBaseDir)
		except:
			print "Directory: "+self.runBaseDir+" already exists. Exiting..."
			sys.exit(1)

		os.chdir(self.runBaseDir)

		#Faz um loop executando tarefas para todos os components incluindo o origin
		components = self.doc.xpath('/job/components/*')
		for component in components:
			#Origin: Cria o diretorio Origin e copia para ele os arquivos basicos para a rodada
			if component.attributes[(None,'id')].nodeValue == "origin":
				try:
					os.mkdir(component.attributes[(None,'id')].nodeValue)
				except:
					pass

				os.chdir(component.attributes[(None,'id')].nodeValue)
				import shutil
				input_xpath = '//job/components/component[@id="origin"]/data/file'
				arquivos = self.doc.xpath(input_xpath)
				for arq_element in arquivos:
#					shutil.copy(dataRepository+'/'+arq_element.attributes[(None,'value')].nodeValue, '.')
					os.symlink(dataRepository+'/'+arq_element.attributes[(None,'value')].nodeValue, arq_element.attributes[(None,'value')].nodeValue)

			#Checa o modulo a ser trabalhado
			modNode = component.xpath('module/text()')
			if (modNode):
				#Cria um diretorio para o processo atual, entra nele e define 
				#uma variavel para o node config e uma para o node input
				os.mkdir(component.attributes[(None,'id')].nodeValue)
				os.chdir(component.attributes[(None,'id')].nodeValue)
				cxml = component.xpath('config')[0]
				ixml = component.xpath('input/*')


				f = open('input.xml','w')
#				f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
				#comeca a escrever o arquivo input.xml
				input_doc = dom.implementation.createRootNode(self.uniqId)
				tag_data = input_doc.createElementNS(xml.EMPTY_NAMESPACE,  'input')
				input_doc.appendChild(tag_data)
				
				#para cada arquivo na lista de inputs
				#adiciona a entrada correspondente no input.xml
				xmlInputError = None
				for inp in ixml:
					input_name = inp.nodeName
					#skip se o input for um config
#					if input_name == "config":
#						continue
					input_source = inp.attributes[(None,'source')].nodeValue
					input_id = inp.attributes[(None,'id')].nodeValue

					input_xpath = '//job/components/component[@id="'+input_source+'"]/data/'+input_name+'[ @id="'+input_id+'"]'
					input_els = self.doc.xpath(input_xpath)
					if len(input_els) == 0:
						xmlInputError = component.attributes[(None,'id')].nodeValue
						continue#continue
					tag_data.appendChild(input_els[0].cloneNode(1))

					file_name = input_els[0].attributes[(None,'value')].nodeValue

					#apos escrever toda a informacao do arquivo atual no input.xml
					#cria um link simbolico do arquivo na pasta origin
					#para a pasta do processo atual
					if input_name == "file":
						os.symlink(str(self.runBaseDir+'/'+input_source+'/'+file_name), './'+file_name)

				if (xmlInputError):
					print "Could not get all required items to start %s" %(xmlInputError)
					continue
					


				#finaliza o arquivo input.xml
				dom.PrettyPrint(input_doc, stream=f)
				f.close()

				#duplica a parte de config do XML para um novo xml config.xml
				f = open('config.xml','w')
				f.write('<?xml version="1.0" encoding="UTF-8"?>\n')

				# The config.xml file also holds a copy of all global definitions
				# which are appended in the form of "global.<id>" 
				# For now, only scalars may be global
				try:
					globals_xpath = '/job/config/global'
					globals_els = self.doc.xpath(globals_xpath)
					for global_conf in globals_els:
						g_val = global_conf.attributes[(None,'value')].nodeValue
						g_attr = global_conf.attributes[(None,'id')].nodeValue
						g_tipo = global_conf.attributes[(None,'type')].nodeValue

						gel = self.doc.createElementNS(xml.EMPTY_NAMESPACE, 'scalar')
						gel.setAttributeNS(None, u'type', g_tipo)
						gel.setAttributeNS(None, u'value', g_val)
						gel.setAttributeNS(None, u'id', 'global.'+g_attr)
						cxml.appendChild(gel)
				except:
					pass # No globals defined

				dom.PrettyPrint(cxml, stream=f)
				f.close()

				#Terminadas as acoes com xml checa o nome do modulo a ser importado pelo python
				#importa o modulo e o executa
#				try:
				modname = component.xpath('module/text()')[0].nodeValue
				m = __import__('components.'+modname)
				mod = getattr(m, modname)
				print "Running module: "+modname
				mod.run()
				print "Finished module: "+modname
#				except:
#					#Nosso modulo falhou por algum motivo ainda nao tratado. 
#					#Seguimos para o proximo modulo.
#					print "modulo", str(mod),"abortado"
#					continue 
##
				#Le o arquivo output.xml e escreve o conteudo no xml original
				outdoc = self.loadXML('output.xml')
				component.appendChild(outdoc.xpath('data')[0])
#
#				dom.PrettyPrint(self.doc)

			os.chdir(self.runBaseDir)

			log = open('productLog.xml', 'w')
			writer = xml.MarkupWriter(log, indent=u"yes")
			writer.startDocument()
			writer.processingInstruction(u'xml-stylesheet', u'type="text/xsl" href="..//productLog.xsl"')

			dic = {
				'classification':'Star/Galaxy Classification',
				'spatial':'Spatial Distribution',
				'psf':'PSF',
				'astrometry':'Astrometry',
				'counts':'Counts',
				'photometry':'Photometry',
				'hyperz':'HyperZ',
				'lephare':'Lephare',
				'color-color':'Color-Color',
				'color-mag':'Color-Magnitude',
				'eros':'EROs',
				'cor2D':'Correlation Function'
				}
			

			X = self.doc

			writer.startElement(u'analysis')

			for module in X.xpath('/job/components/component'):
				for name in dic.keys():
					if module.attributes[(None,'id')].nodeValue.startswith(name):
						writer.startElement(u'group', attributes={u'name':u''+dic[name]})
						filegal = []
						filestar = []
						fileqso = []
						fileall = []
						fileold = []
						filedusty = []
						fileeros = []
						for file in module.xpath('data/file'):
							if file.attributes[(None,'id')].nodeValue.find('gal') != -1:
								filegal.append(file)
							elif file.attributes[(None,'id')].nodeValue.find('star') != -1:
								filestar.append(file)
							elif file.attributes[(None,'id')].nodeValue.find('qso') != -1:
								fileqso.append(file)
							elif file.attributes[(None,'id')].nodeValue.find('dusty') != -1:
								filedusty.append(file)
							elif file.attributes[(None,'id')].nodeValue.find('old') != -1:
								fileold.append(file)
							elif file.attributes[(None,'id')].nodeValue.find('eros') != -1:
								fileeros.append(file)
							else:
								fileall.append(file)
						if len(filegal) > 0:
							writer.startElement(u'subgroup', attributes={u'name':u'Galaxy'})
							for file in filegal:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
							writer.endElement(u'subgroup')
						if len(filestar) > 0:
							writer.startElement(u'subgroup', attributes={u'name':u'Star'})
							for file in filestar:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
							writer.endElement(u'subgroup')
						if len(fileqso) > 0:
							writer.startElement(u'subgroup', attributes={u'name':u'QSO'})
							for file in fileqso:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
							writer.endElement(u'subgroup')
						if len(fileeros) > 0:
							writer.startElement(u'subgroup', attributes={u'name':u'All'})
							for file in fileeros:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
							writer.endElement(u'subgroup')
						if len(fileold) > 0:
							writer.startElement(u'subgroup', attributes={u'name':u'Old'})
							for file in fileold:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
							writer.endElement(u'subgroup')
						if len(filedusty) > 0:
							writer.startElement(u'subgroup', attributes={u'name':u'Dusty'})
							for file in filedusty:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
							writer.endElement(u'subgroup')
						if len(fileall) > 0:
							for file in fileall:
								if file.attributes[(None,'type')].nodeValue == 'plot':
									writer.startElement(u'plot')
									writer.simpleElement(u'image', content=u''+module.attributes[(None,'id')].nodeValue+'/'+file.attributes[(None,'value')].nodeValue)
									writer.simpleElement(u'name', content=u''+file.attributes[(None,'name')].nodeValue)
									writer.simpleElement(u'sub', content=u''+file.attributes[(None,'sub')].nodeValue)
									writer.endElement(u'plot')
						writer.endElement(u'group')

			writer.endElement(u'analysis')
			writer.endDocument()

