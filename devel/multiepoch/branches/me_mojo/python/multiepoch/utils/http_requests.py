'''
Library module providing an easy-to-use API for http requests. 

Loads credentials and WebAPI settings from a .desservices file in the users
home directory. Needs therein a section called "filearchive".

USAGE:
``````
- download file from address:
    http_requests.download_file('http://www.blabla.net/foo.xyz', 'blabla.xyz')


:author: michael h graber, michael.graber@fhnw.ch
'''

import urllib, urllib2
from base64 import b64encode


# -----------------------------------------------------------------------------

# import the credentials via serviceaccess from the local .desservices file if
# possible

try:
    from despyserviceaccess import serviceaccess
    creds = serviceaccess.parse(None, 'http-desarchive')
    USERNAME = creds['user']
    PASSWORD = creds['passwd']
    URL = creds['url']
except:
    USERNAME = None
    PASSWORD = None
    URL = None 
    print "WARNING: could not load credentials from .desservices.ini file -- make sure "

# -----------------------------------------------------------------------------

def download_file(url, filename):
    '''
    '''
    req = Request()
    req.download_file(url, filename)


class Request(object):
    '''
    '''

    def __init__(self, auth=(USERNAME, PASSWORD)):
        self.auth = auth
        self.url = None 
        self.response = None
        self.error_status = (False, '')

    def POST(self, url=URL, data=None):
        ''' '''
        if not type(data)==dict:
            raise ValueError(('The data kwarg needs to be set and of type '
                'dictionary.'))
        else:
            self.data = data
        if not url:
            raise ValueError('You need to provide an url kwarg.')
        else:
            self.url = url

        urllib_req = urllib2.Request(self.url)
        if any(self.auth):
            urllib_req.add_header('Authorization',
                    'Basic ' + b64encode(self.auth[0]+':'+self.auth[1]))
        try:
            self.response = urllib2.urlopen(urllib_req,
                    urllib.urlencode(self.data))
        except Exception, e:
            self.error_status = (True, str(e))

    def get_read(self, url=URL):
        ''' '''
        if not url:
            raise ValueError('You need to provide an url kwarg.')
        else:
            self.url = url

        urllib_req = urllib2.Request(self.url)
        if any(self.auth):
            urllib_req.add_header('Authorization',
                    'Basic ' + b64encode(self.auth[0]+':'+self.auth[1]))
        try:
            self.response = urllib2.urlopen(urllib_req)
            return self.response.read()
        except Exception, e:
            self.error_status = (True, str(e))

    def download_file(self, url, filename):
        ''' '''
        with open(filename, 'wb') as f:
            f.write(self.get_read(url))

    def GET(self, url=URL, params={}):
        ''' '''
        if not url:
            raise ValueError('You need to provide an url kwarg.')
        else:
            self.url = url

        url_params = '?'+'&'.join([str(k)+'='+str(v) for k, v in 
                                                 params.iteritems()])
        urllib_req = urllib2.Request(self.url+url_params)
        if any(self.auth):
            urllib_req.add_header('Authorization',
                    'Basic ' + b64encode(self.auth[0]+':'+self.auth[1]))
        try:
            self.response = urllib2.urlopen(urllib_req)
        except Exception, e:
            self.error_status = (True, str(e))
