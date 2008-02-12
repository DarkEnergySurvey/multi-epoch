import Ft.Xml.Domlette as dom
import re

def loadXML(file):
        #file_string = ''.join(open(file).readlines()).replace('\n','').replace('\t','')
        file_string = open(file).read().replace('\n','').replace('\t','')
        s1 = re.sub('[\t\n]', '', file_string)
        s2 = re.sub('\ +', ' ', s1)
        s3 = re.sub('> ', '>', s2)
        file_string = re.sub(' <', '<', s3)
        return  dom.NonvalidatingReader.parseString(file_string, 'bogus') #bogus word added here because 4Suite is pretty insistent about wanting base URIs for source documents
