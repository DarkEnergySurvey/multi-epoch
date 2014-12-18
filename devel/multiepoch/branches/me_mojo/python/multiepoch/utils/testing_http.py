#!/usr/bin/env python

import os,sys
import http_requests


url1 = "https://desar2.cosmology.illinois.edu/DESFiles/new_archive/Prototype/refacthome/ACT/firstcut/20121207-r1155/D00158994/p01/red/D00158994_g_c01_r1155p01_immasked.fits"
url2 = "https://desar2.cosmology.illinois.edu/DESFiles/new_archive/Prototype/refacthome/ACT/firstcut/20121207-r1155/D00158994/p01/red/D00158994_g_c03_r1155p01_immasked.fits"
url3 = "https://desar2.cosmology.illinois.edu/DESFiles/new_archive/Prototype/refacthome/ACT/firstcut/20121207-r1155/D00158994/p01/red/D00158994_g_c04_r1155p01_immasked.fits"

#h = http_requests.Request()
#h.download_file(url1,"test1.fits")
#h.download_file(url2,"test2.fits")
#h.download_file(url3,"test3.fits")

http_requests.download_file(url1,"test1.fits")
http_requests.download_file(url2,"test2.fits")
http_requests.download_file(url3,"test3.fits")

sys.exit()
