import json
import xml.etree.ElementTree as ET
import requests
import sys
import math
from datetime import timezone
from datetime import datetime

def get_HK_ior(H_ratio, C_ratio):
    out_iorh = H_ratio
    out_iorc = H_ratio
    nowType = ""
    lowRatio = C_ratio
    nowRatio = H_ratio
    highRatio = C_ratio
    if (H_ratio <= 1000 and C_ratio <= 1000):
        out_iorh = math.floor(H_ratio / 10 + 0.0001) * 10
        out_iorc = math.floor(C_ratio / 10 + 0.0001) * 10
        return out_iorh, out_iorc
        
    line = 2000 - (H_ratio + C_ratio)
    if (H_ratio > C_ratio):
        lowRatio = C_ratio
        nowType = "C"
    else:
        lowRatio = H_ratio
        nowType = "H"
        
    if (2000 - line - lowRatio > 1000):
        nowRatio = (lowRatio + line) * -1
    else:
        nowRatio = 2000 - line - lowRatio
        
    if (nowRatio < 0):
        highRatio = math.floor(abs(1000 / nowRatio) * 1000)
    else:
        highRatio = 2000 - line - nowRatio
        
    if (nowType == "H"):
        out_iorh = math.floor(lowRatio / 10 + 0.0001) * 10
        out_iorc = math.floor(highRatio / 10 + 0.0001) * 10 
    else:
        out_iorh = math.floor(highRatio / 10 + 0.0001) * 10
        out_iorc = math.floor(lowRatio / 10 + 0.0001) * 10
        
    return out_iorh, out_iorc
    
def convertHC(iorH, iorC):
    iorH = math.floor(iorH * 1000 + 0.001) / 1000
    iorC = math.floor(iorC * 1000 + 0.001) / 1000
    if (iorH < 11):
        iorH *= 1000
    if (iorC < 11):
        iorC *= 1000

    iorH, iorC = get_HK_ior(iorH, iorC)
    iorH /= 1000
    iorC /= 1000
    iorH += 1
    iorC += 1
    return iorH, iorC
    
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
        'ltype':'3',
        'isRB':'N',
        'lid':argv[2],
        'ecid':argv[3]
    }

    response = requests.post(url=url, cookies=cookies, data=data)
    doc = ET.fromstring(response.text)
    tree = ET.ElementTree(doc)
    data = {}
    data['handicap_count'] = 0
    data['1sthalf_count'] = 0
    handicap_cnt = 0
    firsthalf_cnt = 0
    for game in tree.findall('game'):
        if(game.get('mode') == 'FT'):
            dt = datetime.fromisoformat(game.find('datetime').text)
            timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
            
            txt_ior_rh = game.find('ior_RH').text
            txt_ior_rc = game.find('ior_RC').text
            txt_ior_ouh = game.find('ior_OUH').text
            txt_ior_ouc = game.find('ior_OUC').text
            txt_ior_hrh = game.find('ior_HRH').text
            txt_ior_hrc = game.find('ior_HRC').text
            txt_ior_houh = game.find('ior_HOUH').text
            txt_ior_houc = game.find('ior_HOUC').text
            if(game.find('ior_RH').text == None):
                txt_ior_rh = '-1'
            if(game.find('ior_RC').text == None):
                txt_ior_rc = '-1'
            if(game.find('ior_OUH').text == None):
                txt_ior_ouh = '-1'
            if(game.find('ior_OUC').text == None):
                txt_ior_ouc = '-1'
            if(game.find('ior_HRH').text == None):
                txt_ior_hrh = '-1'
            if(game.find('ior_HRC').text == None):
                txt_ior_hrc = '-1'
            if(game.find('ior_HOUH').text == None):
                txt_ior_houh = '-1'
            if(game.find('ior_HOUC').text == None):
                txt_ior_houc = '-1'
                
            ior_rh = round(float(txt_ior_rh), 3)
            ior_rc = round(float(txt_ior_rc), 3)
            ior_ouh = round(float(txt_ior_ouh), 3)
            ior_ouc = round(float(txt_ior_ouc), 3)
            ior_hrh = round(float(txt_ior_hrh), 3)
            ior_hrc = round(float(txt_ior_hrc), 3)
            ior_houh = round(float(txt_ior_houh), 3)
            ior_houc = round(float(txt_ior_houc), 3)
            
            ior_rh, ior_rc = convertHC(ior_rh, ior_rc)
            ior_ouh, ior_ouc = convertHC(ior_ouh, ior_ouc)
            ior_hrh, ior_hrc = convertHC(ior_hrh, ior_hrc)
            ior_houh, ior_houc = convertHC(ior_houh, ior_houc)
            
            ior_rh = round(ior_rh, 3)
            ior_rc = round(ior_rc, 3)
            ior_ouh = round(ior_ouh, 3)
            ior_ouc = round(ior_ouc, 3)
            ior_hrh = round(ior_hrh, 3)
            ior_hrc = round(ior_hrc, 3)
            ior_houh = round(ior_houh, 3)
            ior_houc = round(ior_houc, 3)
            
            ratio_r = game.find('ratio').text
            if(ratio_r != None):
                ratio_r_list = ratio_r.split(' / ')
                if(len(ratio_r_list) > 1) :
                    ratio_r = str((float(ratio_r_list[0]) + float(ratio_r_list[1])) / 2)
            ratio_hr = game.find('hratio').text
            if(ratio_hr != None):
                ratio_hr_list = ratio_hr.split(' / ')
                if(len(ratio_hr_list) > 1) :
                    ratio_hr = str((float(ratio_hr_list[0]) + float(ratio_hr_list[1])) / 2)
            ratio_ouo = game.find('ratio_o').text
            if(ratio_ouo != None):
                ratio_ouo_list = ratio_ouo.split(' / ')
                if(len(ratio_ouo_list) > 1) :
                    ratio_ouo = str((float(ratio_ouo_list[0]) + float(ratio_ouo_list[1])) / 2)
            ratio_ouu = game.find('ratio_u').text
            if(ratio_ouu != None):
                ratio_ouu_list = ratio_ouu.split(' / ')
                if(len(ratio_ouu_list) > 1) :
                    ratio_ouu = str((float(ratio_ouu_list[0]) + float(ratio_ouu_list[1])) / 2)
            ratio_houo = game.find('ratio_ho').text
            if(ratio_houo != None):
                ratio_houo_list = ratio_houo.split(' / ')
                if(len(ratio_houo_list) > 1) :
                    ratio_houo = str((float(ratio_houo_list[0]) + float(ratio_houo_list[1])) / 2)
            ratio_houu = game.find('ratio_hu').text
            if(ratio_houu != None):
                ratio_houu_list = ratio_houu.split(' / ')
                if(len(ratio_houu_list) > 1) :
                    ratio_houu = str((float(ratio_houu_list[0]) + float(ratio_houu_list[1])) / 2)
                    
            data['TEAM_H'] = game.find('team_h').text
            data['TEAM_c'] = game.find('team_c').text
            data['LEAGUE'] = game.find('league').text
            data["DATETIME"] = str(int(timestamp))
            handicap_cnt += 1
            firsthalf_cnt += 1
            data['handicap_count'] = handicap_cnt
            data['1sthalf_count'] = firsthalf_cnt
            data['handicap_' + str(handicap_cnt)] = {
                "STRONG": game.find('hstrong').text,
                "RATIO_R": ratio_r,
                "ior_RH": str(ior_rh),
                "ior_RC": str(ior_rc),
                "RATIO_OUO": ratio_ouo,
                "RATIO_OUU": ratio_ouu,
                "ior_OUH": str(ior_ouc),
                "ior_OUC": str(ior_ouh)
            }
            data['1sthalf_' + str(firsthalf_cnt)] = {
                "STRONG": game.find('hstrong').text,
                "RATIO_HR": ratio_hr,
                "ior_HRH": str(ior_hrh),
                "ior_HRC": str(ior_hrc),
                "RATIO_HOUO": ratio_houo,
                "RATIO_HOUU": ratio_houu,
                "ior_HOUH": str(ior_houc),
                "ior_HOUC": str(ior_houh)
            }
        if(game.get('mode') == 'CN'):
            ratio_r = game.find('ratio').text
            if(ratio_r != None):
                ratio_r_list = ratio_r.split(' / ')
                if(len(ratio_r_list) > 1) :
                    ratio_r = str((float(ratio_r_list[0]) + float(ratio_r_list[1])) / 2)
            txt_ior_rh = game.find('ior_RH').text
            txt_ior_rc = game.find('ior_RC').text
            txt_ior_ouh = game.find('ior_OUH').text
            txt_ior_ouc = game.find('ior_OUC').text
            txt_ior_hrh = game.find('ior_HRH').text
            txt_ior_hrc = game.find('ior_HRC').text
            txt_ior_houh = game.find('ior_HOUH').text
            txt_ior_houc = game.find('ior_HOUC').text
            if(game.find('ior_RH').text == None):
                txt_ior_rh = '-1'
            if(game.find('ior_RC').text == None):
                txt_ior_rc = '-1'
            if(game.find('ior_OUH').text == None):
                txt_ior_ouh = '-1'
            if(game.find('ior_OUC').text == None):
                txt_ior_ouc = '-1'
            if(game.find('ior_HRH').text == None):
                txt_ior_hrh = '-1'
            if(game.find('ior_HRC').text == None):
                txt_ior_hrc = '-1'
            if(game.find('ior_HOUH').text == None):
                txt_ior_houh = '-1'
            if(game.find('ior_HOUC').text == None):
                txt_ior_houc = '-1'
            ior_rh = round(float(txt_ior_rh), 3)
            ior_rc = round(float(txt_ior_rc), 3)
            ior_ouh = round(float(txt_ior_ouh), 3)
            ior_ouc = round(float(txt_ior_ouc), 3)
            ior_hrh = round(float(txt_ior_hrh), 3)
            ior_hrc = round(float(txt_ior_hrc), 3)
            ior_houh = round(float(txt_ior_houh), 3)
            ior_houc = round(float(txt_ior_houc), 3)
            ior_rh, ior_rc = convertHC(ior_rh, ior_rc)
            ior_ouh, ior_ouc = convertHC(ior_ouh, ior_ouc)
            ior_hrh, ior_hrc = convertHC(ior_hrh, ior_hrc)
            ior_houh, ior_houc = convertHC(ior_houh, ior_houc)
            ior_rh = round(ior_rh, 3)
            ior_rc = round(ior_rc, 3)
            ior_ouh = round(ior_ouh, 3)
            ior_ouc = round(ior_ouc, 3)
            ior_hrh = round(ior_hrh, 3)
            ior_hrc = round(ior_hrc, 3)
            ior_houh = round(ior_houh, 3)
            ior_houc = round(ior_houc, 3)
            data['CN_DATA'] = {
                "STRONG": game.find('hstrong').text,
                "RATIO_R": ratio_r,
                "ior_RH": str(ior_rh),
                "ior_RC": str(ior_rc),
                "RATIO_OUO": game.find('ratio_o').text,
                "RATIO_OUU": game.find('ratio_u').text,
                "ior_OUH": str(ior_ouc),
                "ior_OUC": str(ior_ouh),
                "RATIO_HR": game.find('hratio').text,
                "ior_HRH": str(ior_hrh),
                "ior_HRC": str(ior_hrc),
                "RATIO_HOUO": game.find('ratio_ho').text,
                "RATIO_HOUU": game.find('ratio_hu').text,
                "ior_HOUH": str(ior_houc),
                "ior_HOUC": str(ior_houh)
            }
        if(game.get('mode') == 'RN'):
            ratio_r = game.find('ratio').text
            if(ratio_r != None):
                ratio_r_list = ratio_r.split(' / ')
                if(len(ratio_r_list) > 1) :
                    ratio_r = str((float(ratio_r_list[0]) + float(ratio_r_list[1])) / 2)
            txt_ior_rh = game.find('ior_RH').text
            txt_ior_rc = game.find('ior_RC').text
            txt_ior_ouh = game.find('ior_OUH').text
            txt_ior_ouc = game.find('ior_OUC').text
            txt_ior_hrh = game.find('ior_HRH').text
            txt_ior_hrc = game.find('ior_HRC').text
            txt_ior_houh = game.find('ior_HOUH').text
            txt_ior_houc = game.find('ior_HOUC').text
            if(game.find('ior_RH').text == None):
                txt_ior_rh = '-1'
            if(game.find('ior_RC').text == None):
                txt_ior_rc = '-1'
            if(game.find('ior_OUH').text == None):
                txt_ior_ouh = '-1'
            if(game.find('ior_OUC').text == None):
                txt_ior_ouc = '-1'
            if(game.find('ior_HRH').text == None):
                txt_ior_hrh = '-1'
            if(game.find('ior_HRC').text == None):
                txt_ior_hrc = '-1'
            if(game.find('ior_HOUH').text == None):
                txt_ior_houh = '-1'
            if(game.find('ior_HOUC').text == None):
                txt_ior_houc = '-1'
            ior_rh = round(float(txt_ior_rh), 3)
            ior_rc = round(float(txt_ior_rc), 3)
            ior_ouh = round(float(txt_ior_ouh), 3)
            ior_ouc = round(float(txt_ior_ouc), 3)
            ior_hrh = round(float(txt_ior_hrh), 3)
            ior_hrc = round(float(txt_ior_hrc), 3)
            ior_houh = round(float(txt_ior_houh), 3)
            ior_houc = round(float(txt_ior_houc), 3)
            ior_rh, ior_rc = convertHC(ior_rh, ior_rc)
            ior_ouh, ior_ouc = convertHC(ior_ouh, ior_ouc)
            ior_hrh, ior_hrc = convertHC(ior_hrh, ior_hrc)
            ior_houh, ior_houc = convertHC(ior_houh, ior_houc)
            ior_rh = round(ior_rh, 3)
            ior_rc = round(ior_rc, 3)
            ior_ouh = round(ior_ouh, 3)
            ior_ouc = round(ior_ouc, 3)
            ior_hrh = round(ior_hrh, 3)
            ior_hrc = round(ior_hrc, 3)
            ior_houh = round(ior_houh, 3)
            ior_houc = round(ior_houc, 3)
            data['RN_DATA'] = {
                "STRONG": game.find('hstrong').text,
                "RATIO_R": ratio_r,
                "ior_RH": str(ior_rh),
                "ior_RC": str(ior_rc),
                "RATIO_OUO": game.find('ratio_o').text,
                "RATIO_OUU": game.find('ratio_u').text,
                "ior_OUH": str(ior_ouc),
                "ior_OUC": str(ior_ouh),
                "RATIO_HR": game.find('hratio').text,
                "ior_HRH": str(ior_hrh),
                "ior_HRC": str(ior_hrc),
                "RATIO_HOUO": game.find('ratio_ho').text,
                "RATIO_HOUU": game.find('ratio_hu').text,
                "ior_HOUH": str(ior_houc),
                "ior_HOUC": str(ior_houh)
            }
    with open('match.json', 'w') as file:
        json.dump(data, file)

if __name__ == "__main__":
    GetMatchInformation(sys.argv)
    
'''
python match_infor_by_id.py today 100028,100182,100286,106023,100103,100710,101194,100276,103285,102737,103166,100816,100064,100065,100030,100235,103979,100142,100049,100111,100173,104045,101916,102529,101811,100448,103484,101354,103457,103714,109364,104523,106142,101366,108941,100265,100277,108232,106950,105779,104969,109191,103336,108445,104377,101320,102297,103522,108943,100796,101245,102907,107242,101322,101743,106246,100116,100123,102772,100078,108688,108981,108279,102794,100899,102814,103479,108473,108047,103380 6618037
'''