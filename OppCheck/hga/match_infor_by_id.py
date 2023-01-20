import json
import xml.etree.ElementTree as ET
import requests
import sys

def GetMatchInformation(argv):
    f = open("Configure.xml", "r")
    config = f.read()
    tree = ET.fromstring(config)
    url = tree.find('Hga038Url').text
    mtree = ET.parse('Configure.xml')
    uid = mtree.find('Uid').text

    cookies={
        'CookieChk':'WQ',
        'box4pwd_notshow_29401625':'Mjk0MDE2MjVfTg==',
        'myGameVer_29401625':'XzIxMTIyOA==',
        'ft_myGame_29401625':'e30=',
        'protocolstr':'aHR0cHM=',
        'loginuser':'WGlhbkRhbjU0',
        'test':'aW5pdA',
        'announcement_29401625_202209':'Mjk0MDE2MjVfTg==',
        'login_29401625':'MTY3MzYyMjQxMg',
        'cu':'Tg=='
    }

    data = {
        'uid':uid,
        'ver':'-3ed5-banner-0116-95881ae8576be7',
        'langx':'en-us',
        'p':'get_game_more',
        'gtype':'ft',
        'showtype':argv[1],
        'ltype':'4',
        'isRB':'N',
        'lid':argv[2],
        'ecid':argv[3]
    }

    response = requests.post(url=url, cookies=cookies, data=data)
    doc = ET.fromstring(response.text)
    tree = ET.ElementTree(doc)
    tree.write('match.xml', encoding="utf-8")

if __name__ == "__main__":
    GetMatchInformation(sys.argv)
    
'''
python match_infor_by_id.py early 100817,101194,102814,100897,100803,103526,100021,100405,100397,103166,103285,103457,100111,103952,100885,109191,107242,100813,107461,103525,103521,102000,104523,101354,101322,102907,101366,108047,104332,103554,109490,108688,101211,104220,103383,107705,108279 6599066
'''