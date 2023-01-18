import scrapy
import json
import time
import xml.etree.ElementTree as ET

from datetime import timezone
from datetime import datetime

from scrapy import signals 
from scrapy import Spider

class Hga038Spider(scrapy.Spider):
    name = "Hga038"
    url = 'https://hga038.com/transform.php?ver=-3ed5-gmail-0112-95881ae8576be7'
    uid = ''
    cn_data = []
    rn_data = []
    main_data = []
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
            for cn_data in self.cn_data:
                if(cn_data['ID'] == main_data['ID']):
                    self.main_data[index]['CN_DATA'] = cn_data['CN_DATA']
            for rn_data in self.rn_data:
                if(rn_data['ID'] == main_data['ID']):
                    self.main_data[index]['RN_DATA'] = rn_data['RN_DATA']
        with open('hga038-data.json', 'w') as file:
            json.dump(self.main_data, file)
        
    def start_requests(self):
        formdata = {
            'p':'chk_login',
            'langx':'en-us',
            'ver':'-3ed5-gmail-0112-95881ae8576be7',
            'username':'XianDan54',
            'password':'Aaaa2222',
            'app':'N',
            'auto':'CIHCID'
        }
        yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.GetLeagueListAll)
        self.crawler.signals.connect(self.spider_idle, signal=signals.spider_idle)
    def GetLeagueListAll(self, response):
        tree = ET.fromstring(response.text)
        self.uid = tree.find('uid').text
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
                    'ltype':'4',
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
                    yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.parse, meta={'type':response.meta['type'], 'league_name':coupon.find('name').text})
                if(coupon.find('name').text.startswith('Matches - ')):
                    yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.parse, meta={'type':response.meta['type'], 'league_name':coupon.find('name').text})
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
        for ec in tree.findall('ec'):
            game = ec.find('game')
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
            ior_rh = round(float(txt_ior_rh) + 1, 3)
            ior_rc = round(float(txt_ior_rc) + 1, 3)
            ior_ouh = round(float(txt_ior_ouh) + 1, 3)
            ior_ouc = round(float(txt_ior_ouc) + 1, 3)
            ior_hrh = round(float(txt_ior_hrh) + 1, 3)
            ior_hrc = round(float(txt_ior_hrc) + 1, 3)
            ior_houh = round(float(txt_ior_houh) + 1, 3)
            ior_houc = round(float(txt_ior_houc) + 1, 3)
            
            
            dt = datetime.fromisoformat('2023-' + game.find('DATETIME').text[:-1])
                
            timestamp = dt.replace(tzinfo=timezone.utc).timestamp()
            if(game.find('DATETIME').text[-1] == 'p'):
                timestamp = timestamp + 43200 + 14400
            
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
                    
            ratio_hr = game.find('RATIO_HR').text
            if(ratio_hr != None):
                ratio_hr_list = ratio_hr.split(' / ')
                if(len(ratio_hr_list) > 1) :
                    ratio_hr = str((float(ratio_hr_list[0]) + float(ratio_hr_list[1])) / 2)
                    
            ratio_r = game.find('RATIO_R').text
            if(ratio_r != None):
                ratio_r_list = ratio_r.split(' / ')
                if(len(ratio_r_list) > 1) :
                    ratio_r = str((float(ratio_r_list[0]) + float(ratio_r_list[1])) / 2)
            self.main_data.append({
                "ID":ec.get('id')[2:],
                "DATETIME":str(int(timestamp)),
                "LEAGUE":game.find('LEAGUE').text,
                "TEAM_H":team_h,
                "TEAM_C":team_c,                             
                "MAIN_DATA":{
                    "STRONG":game.find('STRONG').text,
                    "RATIO_R":ratio_r,
                    "IOR_RH":str(ior_rh),
                    "IOR_RC":str(ior_rc),
                    "RATIO_OUO":game.find('RATIO_OUO').text,
                    "RATIO_OUU":game.find('RATIO_OUU').text,
                    "IOR_OUH":str(ior_ouc),
                    "IOR_OUC":str(ior_ouh),
                    "RATIO_HR":ratio_hr,
                    "IOR_HRH":str(ior_hrh),
                    "IOR_HRC":str(ior_hrc),
                    "RATIO_HOUO":game.find('RATIO_HOUO').text,
                    "RATIO_HOUU":game.find('RATIO_HOUU').text,
                    "IOR_HOUH":str(ior_houc),
                    "IOR_HOUC":str(ior_houh)
                }
            })
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
                'ltype':'4',
                'is_rb':'N',
                'ts':'1673795767209',
                'isClick':'Y'
            }
            formdata['model'] = 'CN'
            yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.getGameCN)
            formdata['model'] = 'RN'
            yield scrapy.FormRequest(url=self.url, method="POST", cookies=self.cookies, formdata=formdata, callback=self.getGameRN)
        print('\t' + '~'*70)
        
    
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
            ior_rh = round(float(txt_ior_rh) + 1, 3)
            ior_rc = round(float(txt_ior_rc) + 1, 3)
            ior_ouh = round(float(txt_ior_ouh) + 1, 3)
            ior_ouc = round(float(txt_ior_ouc) + 1, 3)
            ior_hrh = round(float(txt_ior_hrh) + 1, 3)
            ior_hrc = round(float(txt_ior_hrc) + 1, 3)
            ior_houh = round(float(txt_ior_houh) + 1, 3)
            ior_houc = round(float(txt_ior_houc) + 1, 3)
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
            ior_rh = round(float(txt_ior_rh) + 1, 3)
            ior_rc = round(float(txt_ior_rc) + 1, 3)
            ior_ouh = round(float(txt_ior_ouh) + 1, 3)
            ior_ouc = round(float(txt_ior_ouc) + 1, 3)
            ior_hrh = round(float(txt_ior_hrh) + 1, 3)
            ior_hrc = round(float(txt_ior_hrc) + 1, 3)
            ior_houh = round(float(txt_ior_houh) + 1, 3)
            ior_houc = round(float(txt_ior_houc) + 1, 3)
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