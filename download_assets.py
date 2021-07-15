#!/usr/bin/env python3

import argparse
import datetime
import os
import sys
from urllib.request import urlretrieve
from zipfile import ZipFile

ASSET_PACK_URL = 'http://vulkan.gpuinfo.org/downloads/vulkan_asset_pack_gltf.zip'
ASSET_PACK_FILE_NAME = 'vulkan_asset_pack_gltf.zip'
ASSET_DOWNLOAD_COMPLETE_FILE = os.path.join('data', 'asset_download_complete')


def reporthook(blocknum, blocksize, totalsize):
    bytesread = blocknum * blocksize
    if totalsize > 0:
        percent = bytesread * 1e2 / totalsize
        s = "\r%5.1f%% (%*d / %d bytes)" % (percent,
                                            len(str(totalsize)), bytesread, totalsize)
        sys.stderr.write(s)
        if bytesread >= totalsize:
            sys.stderr.write("\n")
    else:
        sys.stderr.write("read %d\n" % (bytesread,))


def download_assets(force_download=False):
    '''Download the assets archive and extract it to the data directory

    :param force_download: If true force the download, if false only download
                           the assets if they have not be downloaded already.
    '''
    if (not force_download) and os.path.isfile(ASSET_DOWNLOAD_COMPLETE_FILE):
        print('Assets were already downloaded')
        return
    print("Downloading asset pack from '%s'" % ASSET_PACK_URL)
    urlretrieve(ASSET_PACK_URL, ASSET_PACK_FILE_NAME, reporthook)
    print("Download finished")
    print("Extracting assets")
    zip = ZipFile(ASSET_PACK_FILE_NAME, 'r')
    zip.extractall("./")
    zip.close()
    print("Extract finished")
    asset_download_complete = open(ASSET_DOWNLOAD_COMPLETE_FILE, mode='w')
    asset_download_complete.write(str(datetime.datetime.now()))
    asset_download_complete.close()


if '__main__' == __name__:
    arg_parser = argparse.ArgumentParser(
        description='Download Sascha Williems Vulkan samples asset files')
    arg_parser.add_argument(
        '--force', dest='force',
        action='store_true', default=False,
        help='Force asset download, even if assets were already downloaded')
    arg_parser.parse_args()
    download_assets()
