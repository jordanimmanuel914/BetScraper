import scrapy
import json
import time
import xml.etree.ElementTree as ET

from datetime import timezone
from datetime import datetime

from scrapy import signals 
from scrapy import Spider
import math

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
    
class Hga038Spider(scrapy.Spider):
    name = "Hga038"
    url = ''
    uid = ''
    cn_data = []
    rn_data = []
    main_data = []
    mix_data = []
    hmix_data = []
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


    def spider_idle(self, spider):
        for index, main_data in enumerate(self.main_data):
            self.main_data[index]['handicap_count'] = 0
            self.main_data[index]['1sthalf_count'] = 0
            
        for index, main_data in enumerate(self.main_data):
            for cn_data in self.cn_data:
                if(cn_data['ID'] == main_data['ID']):
                    self.main_data[index]['CN_DATA'] = cn_data['CN_DATA']
            for rn_data in self.rn_data:
                if(rn_data['ID'] == main_data['ID']):
                    self.main_data[index]['RN_DATA'] = rn_data['RN_DATA']
            for mix_data in self.mix_data:
                if(mix_data['ID'] == main_data['ID']):
                    cnt = self.main_data[index]['handicap_count'] + 1
                    self.main_data[index]['handicap_count'] = cnt
                    self.main_data[index]['handicap_' + str(cnt)] = mix_data[list(mix_data.keys())[1]]
            for hmix_data in self.hmix_data:
                if(hmix_data['ID'] == main_data['ID']):
                    cnt = self.main_data[index]['1sthalf_count'] + 1
                    self.main_data[index]['1sthalf_count'] = cnt
                    self.main_data[index]['1sthalf_' + str(cnt)] = hmix_data[list(hmix_data.keys())[1]]
        with open('hga038-data.json', 'w') as file:
            json.dump(self.main_data, file)
        
    def start_requests(self):
        f = open("Configure.xml", "r")
        config = f.read()
        tree = ET.fromstring(config)
        self.url = tree.find('Hga038Url').text
        formdata = {
            'p':'chk_login',
            'langx':'en-us',
            'ver':'-3ed5-gmail-0112-95881ae8576be7',
            'username':tree.find('Name').text,
            'password':tree.find('Password').text,
            'app':'N',
            'auto':'CIHCID'
        }
        yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.GetLeagueListAll)
        self.crawler.signals.connect(self.spider_idle, signal=signals.spider_idle)
    def GetLeagueListAll(self, response):
        tree = ET.fromstring(response.text)
        self.uid = tree.find('uid').text
        mtree = ET.parse('Configure.xml')
        mtree.find('Uid').text = self.uid
        mtree.write('Configure.xml')
        formdata = {
            'p':'get_league_list_All',
            'uid':self.uid,
            'ver':'-3ed5-gmail-0112-95881ae8576be7',
            'langx':'en-us',
            'gtype':'FT',
            'FS':'N',
            'showtype':'ft',
            'date':'0',
            'ts':'1673795767209',
            'nocp':'N'
        }
        yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.GetMatchInformation, meta={'type':'today'})
        formdata = {
            'p':'get_league_list_All',
            'uid':self.uid,
            'ver':'-3ed5-gmail-0112-95881ae8576be7',
            'langx':'en-us',
            'gtype':'FT',
            'FS':'N',
            'showtype':'fu',
            'date':'all',
            'ts':'1673795767209',
            'nocp':'N'
        }
        yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.GetMatchInformation, meta={'type':'early'})
        
    def GetMatchInformation(self, response):
        print('-'*70)
        print(response.meta['type'])
        print('\tPOPULAR')
        xml_text = response.text.replace("&", "&#38;")
        tree = ET.fromstring(xml_text)
        if len(tree.findall('coupons')) > 0:
            coupons = tree.find('coupons')
            for coupon in coupons.findall('coupon'):
                print('\t\t'+coupon.find('name').text)
                formdata = {
                    'uid':self.uid,
                    'ver':'-3ed5-gmail-0112-95881ae8576be7',
                    'langx':'en-us',
                    'p':'get_game_list',
                    'date':'0',
                    'gtype':'ft',
                    'showtype':response.meta['type'],
                    'rtype':'r',
                    'ltype':'3',
                    'lid':coupon.find('lid').text,
                    'action':'clickCoupon',
                    'sorttype':'L',
                    'isFantasy':'N',
                    'ts':'1673795767209'
                }
                if(response.meta['type'] == 'early'):
                    formdata['date'] = 'all'
                    if(coupon.find('field').text == 'cp1'):
                        formdata['date'] = '1'
                if(coupon.find('name').text == "Today's Matches"):
                    yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.parse, meta={'type':response.meta['type'], 'league_name':coupon.find('name').text, 'lid':coupon.find('lid').text})
                if(coupon.find('name').text.startswith('Matches - ')):
                    yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.parse, meta={'type':response.meta['type'], 'league_name':coupon.find('name').text, 'lid':coupon.find('lid').text})
            classifier = tree.find('classifier')
            for region in classifier.findall('region'):
                print('\t' + region.get('name'))
                for league in region.findall('league'):
                    print('\t\t' + league.get('name'))
        print('-'*70)
        
    def parse(self, response):
        print('\t' + '~'*70)
        print(response.meta['league_name'])
        print('\t' + response.meta['type'])
        tree = ET.fromstring(response.text)
        uid=0
        for ec in tree.findall('ec'):
            game = ec.find('game')
            dt = datetime.fromisoformat('2023-' + game.find('DATETIME').text[:-1])
                
            timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
            timestamp += 14400
            if(game.find('DATETIME').text[-1] == 'p' and game.find('DATETIME').text.find('12:') == -1):
                timestamp = timestamp + 43200
            
            prefix = {"AC ", "AFC ", "BSC ", "BK ", "CA ", "CD ", "CF ", "CR ", "CS ", "CSD ", "FC ", "IA ", "RC ", "SA ", "SC ", "SV ", "JF ", "GD ", "Club "}
            suffix = {" AC", " AFC", " BSC", " BK", " CA", " CD", " CF", " CR", " CS", " CSD", " FC", " IA", " RC", " SA", " SC", " SV", " JF", " GD", " Club", " (W)"}
            team_h = game.find('TEAM_H').text
            team_c = game.find('TEAM_C').text
            for fix in prefix:
                if (team_h.startswith(fix)):
                    team_h = team_h[len(fix):]
                if (team_c.startswith(fix)):
                    team_c = team_c[len(fix):]
            for fix in suffix:
                if (team_h.endswith(fix)):
                    team_h = team_h[:len(team_h)-len(fix)]
                if (team_c.endswith(fix)):
                    team_c = team_c[:len(team_c)-len(fix)]
            lid_list = response.meta['lid'].split(',')
           
            self.main_data.append({
                "ID":ec.get('id')[2:],
                "showtype":response.meta['type'],
                "lid": response.meta['lid'],
                "DATETIME":str(int(timestamp)),
                "LEAGUE":game.find('LEAGUE').text,
                "TEAM_H":team_h,
                "TEAM_C":team_c,
                "uid":lid_list[uid]
            })
            uid+=1
            formdata = {
                'uid':self.uid,
                'ver':'-3ed5-gmail-0112-95881ae8576be7',
                'langx':'en-us',
                'p':'get_game_OBT',
                'gtype':'ft',
                'showtype':response.meta['type'],
                'isEarly':'N',
                'model':'CN',
                'ecid':ec.get('id')[2:],
                'ltype':'3',
                'is_rb':'N',
                'ts':'1673795767209',
                'isClick':'Y'
            }
            ou_cnt = int(game.find('OU_COUNT').text)
            cn_cnt = int(game.find('CN_COUNT').text)
            rn_cnt = int(game.find('RN_COUNT').text)
            if (ou_cnt > 0):
                formdata['model'] = 'OU|MIX'
                yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.getGameMIX)
                formdata['model'] = 'OU|HMIX'
                yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.getGameHMIX)
            if (cn_cnt > 0):
                formdata['model'] = 'CN'
                yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.getGameCN)
            if (rn_cnt > 0):
                formdata['model'] = 'RN'
                yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.getGameRN)
        print('\t' + '~'*70)
        
    def getGameMIX(self, response):
        tree = ET.fromstring(response.text)
        if len(tree.findall('ec')) > 0:
            for game in tree.find('ec').findall('game'):
                txt_ior_rh = game.find('IOR_RH').text
                txt_ior_rc = game.find('IOR_RC').text
                txt_ior_ouh = game.find('IOR_OUH').text
                txt_ior_ouc = game.find('IOR_OUC').text
                if(game.find('IOR_RH').text == None):
                    txt_ior_rh = '-1'
                if(game.find('IOR_RC').text == None):
                    txt_ior_rc = '-1'
                if(game.find('IOR_OUH').text == None):
                    txt_ior_ouh = '-1'
                if(game.find('IOR_OUC').text == None):
                    txt_ior_ouc = '-1'
                ior_rh = round(float(txt_ior_rh), 3)
                ior_rc = round(float(txt_ior_rc), 3)
                ior_ouh = round(float(txt_ior_ouh), 3)
                ior_ouc = round(float(txt_ior_ouc), 3)
                ior_rh, ior_rc = convertHC(ior_rh, ior_rc)
                ior_ouh, ior_ouc = convertHC(ior_ouh, ior_ouc)
                ior_rh = round(ior_rh, 3)
                ior_rc = round(ior_rc, 3)
                ior_ouh = round(ior_ouh, 3)
                ior_ouc = round(ior_ouc, 3)
                self.mix_data.append({
                    "ID":tree.find('ec').get('id'),
                    game.get('id'):{
                        "STRONG":game.find('STRONG').text,
                        "RATIO_R":game.find('RATIO_R').text,
                        "IOR_RH":str(ior_rh),
                        "IOR_RC":str(ior_rc),
                        "RATIO_OUO":game.find('RATIO_OUO').text,
                        "RATIO_OUU":game.find('RATIO_OUU').text,
                        "IOR_OUH":str(ior_ouc),
                        "IOR_OUC":str(ior_ouh)
                    }
                })
            
    def getGameHMIX(self, response):
        tree = ET.fromstring(response.text)
        if len(tree.findall('ec')) > 0:
            for game in tree.find('ec').findall('game'):
                txt_ior_hrh = game.find('IOR_HRH').text
                txt_ior_hrc = game.find('IOR_HRC').text
                txt_ior_houh = game.find('IOR_HOUH').text
                txt_ior_houc = game.find('IOR_HOUC').text
                if(game.find('IOR_HRH').text == None):
                    txt_ior_hrh = '-1'
                if(game.find('IOR_HRC').text == None):
                    txt_ior_hrc = '-1'
                if(game.find('IOR_HOUH').text == None):
                    txt_ior_houh = '-1'
                if(game.find('IOR_HOUC').text == None):
                    txt_ior_houc = '-1'
                ior_hrh = round(float(txt_ior_hrh), 3)
                ior_hrc = round(float(txt_ior_hrc), 3)
                ior_houh = round(float(txt_ior_houh), 3)
                ior_houc = round(float(txt_ior_houc), 3)
                ior_hrh, ior_hrc = convertHC(ior_hrh, ior_hrc)
                ior_houh, ior_houc = convertHC(ior_houh, ior_houc)
                ior_hrh = round(ior_hrh, 3)
                ior_hrc = round(ior_hrc, 3)
                ior_houh = round(ior_houh, 3)
                ior_houc = round(ior_houc, 3)
                self.hmix_data.append({
                    "ID":tree.find('ec').get('id'),
                    game.get('id'):{
                        "STRONG":game.find('STRONG').text,
                        "RATIO_HR":game.find('RATIO_HR').text,
                        "IOR_HRH":str(ior_hrh),
                        "IOR_HRC":str(ior_hrc),
                        "RATIO_HOUO":game.find('RATIO_HOUO').text,
                        "RATIO_HOUU":game.find('RATIO_HOUU').text,
                        "IOR_HOUH":str(ior_houc),
                        "IOR_HOUC":str(ior_houh)
                    }
                })
                
    def getGameCN(self, response):
        tree = ET.fromstring(response.text)
        if len(tree.findall('ec')) > 0:
            game = tree.find('ec').find('game')
            txt_ior_rh = game.find('IOR_RH').text
            txt_ior_rc = game.find('IOR_RC').text
            txt_ior_ouh = game.find('IOR_OUH').text
            txt_ior_ouc = game.find('IOR_OUC').text
            txt_ior_hrh = game.find('IOR_HRH').text
            txt_ior_hrc = game.find('IOR_HRC').text
            txt_ior_houh = game.find('IOR_HOUH').text
            txt_ior_houc = game.find('IOR_HOUC').text
            if(game.find('IOR_RH').text == None):
                txt_ior_rh = '-1'
            if(game.find('IOR_RC').text == None):
                txt_ior_rc = '-1'
            if(game.find('IOR_OUH').text == None):
                txt_ior_ouh = '-1'
            if(game.find('IOR_OUC').text == None):
                txt_ior_ouc = '-1'
            if(game.find('IOR_HRH').text == None):
                txt_ior_hrh = '-1'
            if(game.find('IOR_HRC').text == None):
                txt_ior_hrc = '-1'
            if(game.find('IOR_HOUH').text == None):
                txt_ior_houh = '-1'
            if(game.find('IOR_HOUC').text == None):
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
            team_h = game.find('TEAM_H').text.replace(' - No. of Corners','')
            team_c = game.find('TEAM_C').text.replace(' - No. of Corners','')
            self.cn_data.append({
                "ID":tree.find('ec').get('id'),
                "LEAGUE":game.find('LEAGUE').text,
                "TEAM_H":team_h,
                "TEAM_C":team_c,
                "CN_DATA":{
                    "STRONG":game.find('STRONG').text,
                    "RATIO_R":game.find('RATIO_R').text,
                    "IOR_RH":str(ior_rh),
                    "IOR_RC":str(ior_rc),
                    "RATIO_OUO":game.find('RATIO_OUO').text,
                    "RATIO_OUU":game.find('RATIO_OUU').text,
                    "IOR_OUH":str(ior_ouc),
                    "IOR_OUC":str(ior_ouh),
                    "RATIO_HR":game.find('RATIO_HR').text,
                    "IOR_HRH":str(ior_hrh),
                    "IOR_HRC":str(ior_hrc),
                    "RATIO_HOUO":game.find('RATIO_HOUO').text,
                    "RATIO_HOUU":game.find('RATIO_HOUU').text,
                    "IOR_HOUH":str(ior_houc),
                    "IOR_HOUC":str(ior_houh)
                }
            })
    def getGameRN(self, response):
        tree = ET.fromstring(response.text)
        if len(tree.findall('ec')) > 0:
            game = tree.find('ec').find('game')
            txt_ior_rh = game.find('IOR_RH').text
            txt_ior_rc = game.find('IOR_RC').text
            txt_ior_ouh = game.find('IOR_OUH').text
            txt_ior_ouc = game.find('IOR_OUC').text
            txt_ior_hrh = game.find('IOR_HRH').text
            txt_ior_hrc = game.find('IOR_HRC').text
            txt_ior_houh = game.find('IOR_HOUH').text
            txt_ior_houc = game.find('IOR_HOUC').text
            if(game.find('IOR_RH').text == None):
                txt_ior_rh = '-1'
            if(game.find('IOR_RC').text == None):
                txt_ior_rc = '-1'
            if(game.find('IOR_OUH').text == None):
                txt_ior_ouh = '-1'
            if(game.find('IOR_OUC').text == None):
                txt_ior_ouc = '-1'
            if(game.find('IOR_HRH').text == None):
                txt_ior_hrh = '-1'
            if(game.find('IOR_HRC').text == None):
                txt_ior_hrc = '-1'
            if(game.find('IOR_HOUH').text == None):
                txt_ior_houh = '-1'
            if(game.find('IOR_HOUC').text == None):
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
            team_h = game.find('TEAM_H').text.replace(' - No. of Corners','')
            team_c = game.find('TEAM_C').text.replace(' - No. of Corners','')
            self.rn_data.append({
                "ID":tree.find('ec').get('id'),
                "LEAGUE":game.find('LEAGUE').text,
                "TEAM_H":team_h,
                "TEAM_C":team_c,
                "RN_DATA":{
                    "STRONG":game.find('STRONG').text,
                    "RATIO_R":game.find('RATIO_R').text,
                    "IOR_RH":str(ior_rh),
                    "IOR_RC":str(ior_rc),
                    "RATIO_OUO":game.find('RATIO_OUO').text,
                    "RATIO_OUU":game.find('RATIO_OUU').text,
                    "IOR_OUH":str(ior_ouc),
                    "IOR_OUC":str(ior_ouh),
                    "RATIO_HR":game.find('RATIO_HR').text,
                    "IOR_HRH":str(ior_hrh),
                    "IOR_HRC":str(ior_hrc),
                    "RATIO_HOUO":game.find('RATIO_HOUO').text,
                    "RATIO_HOUU":game.find('RATIO_HOUU').text,
                    "IOR_HOUH":str(ior_houc),
                    "IOR_HOUC":str(ior_houh)
                }
            })