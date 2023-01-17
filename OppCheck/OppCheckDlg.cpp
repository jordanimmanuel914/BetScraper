
// OppCheckDlg.cpp : implementation file
//

#include "stdafx.h"
#include "OppCheck.h"
#include "OppCheckDlg.h"
#include "afxdialogex.h"
#undef max
#include "include/rapidjson/document.h"
//#include "include/rapidjson/filereadstream.h"
#include "include/curl/curl.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <algorithm>
#include <vector>

using namespace std;
using namespace rapidjson;

#define PLAYER_WIN	 0
#define LOSE		 1
#define STAKE_REFUND 2
#define HALF_WIN	 3
#define HALF_LOSE	 4
#define CHECK_PARAMS	9
#define MAX_MIDDLES	50
typedef struct middle_inf {
	float odd1;
	float odd2;
	float handi1;
	float handi2;
	float middle;
	int nalt;	//if 1 then this is alternative line.
	middle_inf(){
		odd1 = 0;
		odd2 = 0;
		handi1 = 0;
		handi2 = 0;
		middle = 0;
		nalt = 0;
	}
};
typedef struct arbit_inf {
	float h1;
	float h2;
	float h1_odd1;
	float h1_odd2;
	float h2_odd1;
	float h2_odd2;
	arbit_inf() {
		h1 = 0;
		h2 = 0;
		h1_odd1 = 0;
		h1_odd2 = 0;
		h2_odd1 = 0;
		h2_odd2 = 0;
	}
};
typedef struct match_inf {
	int hga_inx;
	int match_id;
	arbit_inf arinf[CHECK_PARAMS];
	int mcnt[CHECK_PARAMS];
	middle_inf inf[CHECK_PARAMS][MAX_MIDDLES];
	match_inf() {
		hga_inx = 0;
		match_id = 0;
		memset(mcnt, 0, sizeof(int) * CHECK_PARAMS);
	}
} MATCH_INF;
// CAboutDlg dialog used for App About

typedef struct state_struct {
	int goal_diff;
	int state1;
	int state2;
} STATE_STRUCT;

string gs_strLastResponse;
vector<match_inf *> g_matches;
int g_updated = 0;

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// COppCheckDlg dialog



COppCheckDlg::COppCheckDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_OPPCHECK_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_status = 0;
}
COppCheckDlg::~COppCheckDlg() {
	clear_match();
}
void COppCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_ORG, m_org);
	DDX_Control(pDX, IDC_PROGRESS1, m_show);
}

BEGIN_MESSAGE_MAP(COppCheckDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &COppCheckDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// COppCheckDlg message handlers

BOOL COppCheckDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	DWORD dwThreadId;
	gs_strLastResponse = "";
	m_hcatch = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)catch_thread, this, 0, &dwThreadId);
	m_hdisplay = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)display_thread, this, 0, &dwThreadId);
	Initlist();
	return TRUE;  // return TRUE  unless you set the focus to a control
}
float COppCheckDlg::get_profit(float odd, float myhandicap, float money, int mygoal, int op_goal) {
	int nv = (int)(myhandicap * 100);	//convert 0.25 to 25
	int nsign = nv < 0 ? -1 : 1;
	int goal = mygoal - op_goal;	//Mygoal - opponents goal
	int nover = nv / 100 * -1;			//goals
	int status = -1;

	nv = nv < 0 ? -nv : nv;

	if (odd <= 1.0) {
		//This is invalid odd;
		return 0;
	}
	if (nv % 100 == 0) {
		if (goal > nover)
			status = PLAYER_WIN;
		if (goal == nover)
			status = STAKE_REFUND;
		if (goal < nover)
			status = LOSE;
	}
	if (nv % 100 == 25) {
		if (goal > nover)
			status = PLAYER_WIN;
		if (goal == nover)
		{
			if (nsign == -1)
				status = HALF_LOSE;
			else
				status = HALF_WIN;
		}
		if (goal < nover)
			status = LOSE;
	}
	if (nv % 100 == 50) {
		if (goal > nover)
			status = PLAYER_WIN;
		if (goal == nover)
		{
			if (nsign == -1)
				status = LOSE;
			else
				status = PLAYER_WIN;
		}
		if (goal < nover)
			status = LOSE;
	}
	if (nv % 100 == 75) {
		if (nsign == -1) {
			if (goal > nover + 1)
				status = PLAYER_WIN;
			if (goal > nover)
				status = HALF_WIN;
			if (goal <= nover)
				status = LOSE;
		}
		else {
			if (goal >= nover)
				status = PLAYER_WIN;
			if (goal == nover - 1)
				status = HALF_LOSE;
			if (goal < nover - 1)
				status = LOSE;
		}
	}
	if (status == LOSE)
		return -money;
	if (status == PLAYER_WIN)
		return odd * money - money;
	if (status == STAKE_REFUND)
		return 0;
	if (status == HALF_WIN)
		return (money / 2 + money / 2 * odd) - money;
	if (status == HALF_LOSE)
		return -money / 2;
	return 0;
}
int COppCheckDlg::checkOU(char t, float v, int goal) {
	int nv = (int)(v * 100);
	int no = nv / 100;
	if (t == 'u' || t == 'U') {
		if (nv % 100 == 0) {
			if (goal < no)
				return PLAYER_WIN;
			if (goal == no)
				return STAKE_REFUND;
			if (goal > no)
				return LOSE;
		}
		if (nv % 100 == 25) {
			if (goal < no)
				return PLAYER_WIN;
			if (goal == no)
				return HALF_WIN;
			if (goal > no)
				return LOSE;
		}
		if (nv % 100 == 50) {
			if (goal <= no)
				return PLAYER_WIN;
			if (goal > no)
				return LOSE;
		}
		if (nv % 100 == 75) {
			if (goal <= no)
				return PLAYER_WIN;
			if (goal == no + 1)
				return HALF_LOSE;
			if (goal > no)
				return LOSE;
		}
	}
	if (t == 'o' || t == 'O') {
		if (nv % 100 == 0) {
			if (goal > no)
				return PLAYER_WIN;
			if (goal == no)
				return STAKE_REFUND;
			if (goal < no)
				return LOSE;
		}
		if (nv % 100 == 25) {
			if (goal > no)
				return PLAYER_WIN;
			if (goal == no)
				return HALF_LOSE;
			if (goal < no)
				return LOSE;
		}
		if (nv % 100 == 50) {
			if (goal > no)
				return PLAYER_WIN;
			if (goal <= no)
				return LOSE;
		}
		if (nv % 100 == 75) {
			if (goal > no + 1)
				return PLAYER_WIN;
			if (goal == no + 1)
				return HALF_WIN;
			if (goal <= no)
				return LOSE;
		}
	}
	return 0;
}
float COppCheckDlg::calc_middle(float stake1, float stake2, float odd1, float odd2, float handicap1, float handicap2) {
	int i, idx = 0, state1, state2, j, ct, ct1;
	int nv1 = (int)(handicap1 * 100);
	int nv2 = (int)(handicap2 * 100);
	struct state_struct stt_array[10];
	float values[10] = { 0 };
	float tmp,favg,tmp1, favg1;


	stt_array[idx].goal_diff = -20;
	stt_array[idx].state1 = LOSE;
	stt_array[idx++].state2 = PLAYER_WIN;
	for (i = -20; i < 20; i++) {
		state1 = checkWin(handicap1, i, 0);
		state2 = checkWin(handicap2, 0, i);
		if (state1 != stt_array[idx - 1].state1 || state2 != stt_array[idx - 1].state2) {
			stt_array[idx].goal_diff = i;
			stt_array[idx].state1 = state1;
			stt_array[idx++].state2 = state2;
		}
	}

	for (i = 0; i < idx; i++) {
		if (stt_array[i].state1 == PLAYER_WIN)
			values[i] = stake1 * odd1;
		if (stt_array[i].state1 == HALF_LOSE)
			values[i] = stake1 / 2;
		if (stt_array[i].state1 == HALF_WIN)
			values[i] = stake1 / 2 + stake1 / 2 * odd1;
		if (stt_array[i].state1 == STAKE_REFUND)
			values[i] = stake1;

		if (stt_array[i].state2 == PLAYER_WIN)
			values[i] += stake2 * odd2;
		if (stt_array[i].state2 == HALF_LOSE)
			values[i] += stake2 / 2;
		if (stt_array[i].state2 == HALF_WIN)
			values[i] += stake2 / 2 + stake2 / 2 * odd2;
		if (stt_array[i].state2 == STAKE_REFUND)
			values[i] += stake2;

		values[i] -= stake1;
		values[i] -= stake2;
		//values[i] = values[i] < 0 ? -values[i] : values[i];
	}
	tmp = 0;
	tmp1 = 0;
	ct = 0;
	ct1 = 0;
	for (i = 0; i < idx; ++i) {
		if (values[i] < 0) {
			tmp += values[i];
			ct++;
		}
		else {
			if (values[i] > tmp1)
				tmp1 = values[i];
			ct1++;
		}
	}
	if (ct == 0)
		return 10000;
	favg = tmp / ct;
	if (tmp1 < fabs(favg)) {
		tmp = tmp1 / fabs(favg);
		return tmp + 1;
	}
	if (ct1 >= 2) {
		tmp = 0;
		for (i = 0; i < idx; i++) {
			if (values[i] > 0) {
				tmp1 = values[i] / fabs(favg);
				tmp+= tmp1;
			}
		}
		return tmp + 1;
	}
	for (i = 0; i < idx; i++) {
		if (values[i] > 0) {
			tmp = values[i] / fabs(favg) + 1;
			break;
		}
	}
	return tmp;
}
float COppCheckDlg::calc_middle_ou(float stake1, float stake2, float odd1, float odd2, float handicap1, float handicap2, char c1, char c2) {
	int i, j, idx = 0,ct,ct1;
	int state1, state2;
	float tmp, favg, tmp1, favg1;
	struct state_struct stt_array[20];
	float values[20] = { 0 };

	state1 = checkOU(c1, handicap1, 0);
	state2 = checkOU(c2, handicap2, 0);
	stt_array[idx].goal_diff = 0;
	stt_array[idx].state1 = state1;
	stt_array[idx++].state2 = state2;

	for (i = 1; i < 50; i++) {
		state1 = checkOU(c1, handicap1, i);
		state2 = checkOU(c2, handicap2, i);

		if (state1 != stt_array[idx - 1].state1 || state2 != stt_array[idx - 1].state2) {
			stt_array[idx].goal_diff = i;
			stt_array[idx].state1 = state1;
			stt_array[idx++].state2 = state2;
		}
	}

	for (i = 0; i < idx; i++) {
		if (stt_array[i].state1 == PLAYER_WIN)
			values[i] = stake1 * odd1;
		if (stt_array[i].state1 == HALF_LOSE)
			values[i] = stake1 / 2;
		if (stt_array[i].state1 == HALF_WIN)
			values[i] = stake1 / 2 + stake1 / 2 * odd1;
		if (stt_array[i].state1 == STAKE_REFUND)
			values[i] = stake1;

		if (stt_array[i].state2 == PLAYER_WIN)
			values[i] += stake2 * odd2;
		if (stt_array[i].state2 == HALF_LOSE)
			values[i] += stake2 / 2;
		if (stt_array[i].state2 == HALF_WIN)
			values[i] += stake2 / 2 + stake2 / 2 * odd2;
		if (stt_array[i].state2 == STAKE_REFUND)
			values[i] += stake2;

		values[i] -= stake1;
		values[i] -= stake2;
		//values[i] = values[i] < 0 ? -values[i] : values[i];
	}
	tmp = 0;
	tmp1 = 0;
	ct = 0;
	ct1 = 0;
	for (i = 0; i < idx; ++i) {
		if (values[i] < 0) {
			tmp += values[i];
			ct++;
		}
		else {
			if (values[i] > tmp1)
				tmp1 = values[i];
			ct1++;
		}
	}
	if (ct == 0)
		return 10000;
	favg = tmp / ct;	
	if (tmp1 < fabs(favg)) {
		tmp = tmp1 / fabs(favg);
		return tmp + 1;
	}
	if (ct1 >= 2) {
		tmp = 0;
		for (i = 0; i < idx; i++) {
			if (values[i] > 0) {
				tmp1 = values[i] / fabs(favg);
				tmp += tmp1;
			}
		}
		return tmp + 1;
	}
	for (i = 0; i < idx; i++) {
		if (values[i] > 0) {
			tmp = values[i] / fabs(favg) + 1;
			break;
		}
	}
	return tmp;
}
float COppCheckDlg::checkArbitrage(float odd1, float odd2) {
	float x = 100;
	float xx = (odd1 * x) / (odd1 + odd2);
	return xx * odd2 - x;
}
float COppCheckDlg::checkArbitrage3(float a1, float a2, float a3) {
	float y = a3 * 100 / (a2 + a3 + (a2*a3 / a1));
	float x = a2 * y / a1;
	return a2*y - 100;
}
int COppCheckDlg::checkWin(float myhandicap, int mygoal, int op_goal) {
	int nv = (int)(myhandicap * 100);	//convert 0.25 to 25
	int nsign = nv < 0 ? -1 : 1;
	int goal = mygoal - op_goal;	//Mygoal - opponents goal
	int nover = nv / 100 * -1;			//goals
	nv = nv < 0 ? -nv : nv;
	if (nv % 100 == 0) {
		if (goal > nover)
			return PLAYER_WIN;
		if (goal == nover)
			return STAKE_REFUND;
		if (goal < nover)
			return LOSE;
	}
	if (nv % 100 == 25) {
		if (goal > nover)
			return PLAYER_WIN;
		if (goal == nover)
		{
			if (nsign == -1)
				return HALF_LOSE;
			else
				return HALF_WIN;
		}
		if (goal < nover)
			return LOSE;
	}
	if (nv % 100 == 50) {
		if (goal > nover)
			return PLAYER_WIN;
		if (goal == nover)
		{
			if (nsign == -1)
				return LOSE;
			else
				return PLAYER_WIN;
		}
		if (goal < nover)
			return LOSE;
	}
	if (nv % 100 == 75) {
		if (nsign == -1) {
			if (goal > nover + 1)
				return PLAYER_WIN;
			if (goal > nover)
				return HALF_WIN;
			if (goal <= nover)
				return LOSE;
		}
		else {
			if (goal >= nover)
				return PLAYER_WIN;
			if (goal == nover - 1)
				return HALF_LOSE;
			if (goal < nover - 1)
				return LOSE;
		}
	}
	return 0;
}
void COppCheckDlg::Initlist() {
	m_org.InsertColumn(0, L"DATE");
	m_org.SetColumnWidth(0, 120);
	
	m_org.InsertColumn(1, L"League");
	m_org.SetColumnWidth(1, 120);
	
	m_org.InsertColumn(2, L"Team(Home)");
	m_org.SetColumnWidth(2, 120);

	m_org.InsertColumn(3, L"Team(Away)");
	m_org.SetColumnWidth(3, 120);

	m_org.InsertColumn(4, L"Arbitrage");
	m_org.SetColumnWidth(4, 80);

	m_org.InsertColumn(5, L"Where");
	m_org.SetColumnWidth(5, 100);

	m_org.InsertColumn(6, L"Odd1");
	m_org.SetColumnWidth(6, 80);

	m_org.InsertColumn(7, L"Odd2");
	m_org.SetColumnWidth(7, 80);

	m_org.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);
}
void COppCheckDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void COppCheckDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COppCheckDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


size_t function_pt(void *ptr, size_t size, size_t nmemb, void * /*stream*/)
{
	gs_strLastResponse += (const char*)ptr;
	return size * nmemb;
}
void COppCheckDlg::GetHGA() {
	system("hga.bat");
	//int k = 0;

}
void COppCheckDlg::GetBet365_Upcoming() {
	int i, ntotal, npage;
	Document d;
	Document doc;
	char strbuff[500] = { 0 };
	CURL *hnd = curl_easy_init();
	
	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	struct curl_slist *headers = NULL;
	curl_easy_setopt(hnd, CURLOPT_CAINFO, "cacert.pem");
	headers = curl_slist_append(headers, "X-RapidAPI-Key: e2fa07ad89msh0857ce6a64b3678p13de20jsn4cdcdb04c8ff");
	headers = curl_slist_append(headers, "X-RapidAPI-Host: betsapi2.p.rapidapi.com");
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, function_pt);        // set a callback to capture the server's response

	curl_easy_setopt(hnd, CURLOPT_URL, "https://betsapi2.p.rapidapi.com/v1/bet365/upcoming?sport_id=1&skip_esports=true&per_page=100");
	CURLcode ret = curl_easy_perform(hnd);
	if (ret == CURLE_OK) {
		d.Parse(gs_strLastResponse.c_str());
		ntotal = d["pager"]["total"].GetInt();
		npage = (ntotal % 100 == 0) ? ntotal / 100 : ntotal / 100 + 1;
		for (i = 2; i < npage; i++) {
			sprintf_s(strbuff, "https://betsapi2.p.rapidapi.com/v1/bet365/upcoming?page=%d&sport_id=1&skip_esports=true&per_page=100", i);
			curl_easy_setopt(hnd, CURLOPT_URL, strbuff);
			gs_strLastResponse += ",\n";
			ret = curl_easy_perform(hnd);
		}
		gs_strLastResponse = "[" + gs_strLastResponse;
		gs_strLastResponse = gs_strLastResponse + "]";
		doc.Parse(gs_strLastResponse.c_str());
		npage = doc[0]["pager"]["total"].GetInt();
		npage = 0;
	}
	curl_slist_free_all(headers);
	curl_easy_cleanup(hnd);
	FILE *fp = fopen("bet365/index.json", "wb");
	fwrite(gs_strLastResponse.c_str(), 1, gs_strLastResponse.length(), fp);
	fclose(fp);
}
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
	
}
void Replaces_String(string &str) {
	int i, len;
	string tmp;
	string a[23] = { "AC ", "AFC ", "BSC ", "BK ", "CA ", "CD ", "CF ", "CR ", "CS ", "CSD ", "FC ", "IA ", "RC ", "SA ", "SC ", "SV ", "JF ", "GD ", "Club ", "U21 ","U20 ","U19 ","Women " };
	string b[24] = { " AC", " AFC", " BSC", " BK", " CA", " CD", " CF", " CR", " CS", " CSD", " FC", " IA", " RC", " SA", " SC", " SV", " JF", " GD", " Club", " (W)", " U21"," U20"," U19"," Women" };
	for (i = 0; i < 23; i++) {
		len = a[i].length();
		if (len < str.length()) {
			tmp = str.substr(0, len);
			if (tmp == a[i])
				str = ReplaceAll(str, a[i], "");
		}
	}
	for (i = 0; i < 24; i++) {
		len = b[i].length();
		if (len < str.length()) {
			tmp = str.substr(str.length() - len, len);
			if (tmp == b[i])
				str = ReplaceAll(str, b[i], "");
		}
	}
}
void COppCheckDlg::clear_match() {
	int size = g_matches.size();
	int i;
	for (i = 0; i < size; i++) {
		match_inf *pinf = g_matches.at(i);
		delete pinf;
	}
	g_matches.clear();
}
float COppCheckDlg::get_handicap_from_string(const char *p, int deli = ',') {
	char buf[40] = { 0 };
	strcpy(buf, p);
	char *pfound = strchr(buf, deli);
	if (pfound == NULL)
		return atof(buf);
	char *nextval = pfound + 1;
	*pfound = 0;
	float f1 = atof(buf);
	float f2 = atof(nextval);
	return (f1 + f2) / 2;
}
void COppCheckDlg::ReadDataDisplay()
{
	int i, j, k, nk;
	Document hga, bet365;
	char *readBuffer1;
	int filelen1, filelen2;
	char *readBuffer2;

	FILE* fhga = fopen("hga/hga038-data.json", "rb"); // non-Windows use "r"
	if (fhga == NULL)
		return;
	fseek(fhga, 0, SEEK_END);
	filelen1 = ftell(fhga);
	fseek(fhga, 0, SEEK_SET);
	readBuffer1 = new char[filelen1 + 1];
	memset(readBuffer1, 0, filelen1 + 1);
	fread(readBuffer1, 1, filelen1, fhga);
	fclose(fhga);

	FILE* fbet = fopen("bet365/index.json", "rb"); // non-Windows use "r"
	if (fbet == NULL) {
		fclose(fhga);
		return;
	}
	
	fseek(fbet, 0, SEEK_END);
	filelen2 = ftell(fbet);
	fseek(fbet, 0, SEEK_SET);
	readBuffer2 = new char[filelen2 + 1];
	memset(readBuffer2, 0, filelen2 + 1);
	fread(readBuffer2, 1, filelen2, fbet);
	fclose(fbet);

	hga.Parse(readBuffer1);
	bet365.Parse(readBuffer2);

	delete readBuffer1;
	delete readBuffer2;

	int nhga = hga.Size();
	int nbet = bet365.Size();
	float h1, h2, stake1, stake2;
	wchar_t buff[200] = { 0 };
	int idx = 0, nalt;
	g_updated = 0;
	clear_match();

	int nstart_time = GetTickCount();
	m_show.SetRange(0, nhga - 1);
	for (i = 0; i < nhga; i++) {
		string sdatetime = hga[i]["DATETIME"].GetString();
		string sleaguename = hga[i]["LEAGUE"].IsNull() ? "null" : hga[i]["LEAGUE"].GetString();
		string shometeam = hga[i]["TEAM_H"].IsNull() ? "null" : hga[i]["TEAM_H"].GetString();
		string sawayteam = hga[i]["TEAM_C"].IsNull() ? "null" : hga[i]["TEAM_C"].GetString();
		string sstrong = hga[i]["STRONG"].IsNull() ? "null" : hga[i]["STRONG"].GetString();
		const Value& main_data = hga[i]["MAIN_DATA"];
		string shandicap = main_data["RATIO_R"].IsNull() ? "null" : main_data["RATIO_R"].GetString();
		string shodd = main_data["IOR_RH"].IsNull() ? "null" : main_data["IOR_RH"].GetString();
		string scodd = main_data["IOR_RC"].IsNull() ? "null" : main_data["IOR_RC"].GetString();
		string sgoalo = main_data["RATIO_OUO"].IsNull() ? "null" : main_data["RATIO_OUO"].GetString();
		string sgoalu = main_data["RATIO_OUU"].IsNull() ? "null" : main_data["RATIO_OUU"].GetString();
		string sgoaloodd = main_data["IOR_OUH"].IsNull() ? "null" : main_data["IOR_OUH"].GetString();
		string sgoaluodd = main_data["IOR_OUC"].IsNull() ? "null" : main_data["IOR_OUC"].GetString();
		string s1sthandicap = main_data["RATIO_HR"].IsNull() ? "null" : main_data["RATIO_HR"].GetString();
		string s1sthodd = main_data["IOR_HRH"].IsNull() ? "null" : main_data["IOR_HRH"].GetString();
		string s1stcodd = main_data["IOR_HRC"].IsNull() ? "null" : main_data["IOR_HRC"].GetString();
		string s1stgoalo = main_data["RATIO_HOUO"].IsNull() ? "null" : main_data["RATIO_HOUO"].GetString();
		string s1stgoalu = main_data["RATIO_HOUU"].IsNull() ? "null" : main_data["RATIO_HOUU"].GetString();
		string s1stgoaloodd = main_data["IOR_HOUH"].IsNull() ? "null" : main_data["IOR_HOUH"].GetString();
		string s1stgoaluodd = main_data["IOR_HOUC"].IsNull() ? "null" : main_data["IOR_HOUC"].GetString();
		string sid = "";
		int ndiff = 0;
		for (j = 0; j < nbet; j++) {
			if (!bet365[j].HasMember("results"))
				continue;
			nk = bet365[j]["results"].Size();
			for (k = 0; k < nk; k++) {
				string stime = bet365[j]["results"][k]["time"].GetString();
				ndiff = abs(atoi(stime.c_str()) - atoi(sdatetime.c_str()));
				if (ndiff > 3600)
					continue;
				string sleague = bet365[j]["results"][k]["league"]["name"].GetString();
				string sh = bet365[j]["results"][k]["home"]["name"].GetString();
				string sc = bet365[j]["results"][k]["away"]["name"].GetString();
				string sh1 = shometeam;
				string sc1 = sawayteam;
				for (auto & c : sh) c = toupper(c);
				for (auto & c : sc) c = toupper(c);
				for (auto & c : sc1) c = toupper(c);
				for (auto & c : sh1) c = toupper(c);

				Replaces_String(sh1);
				Replaces_String(sc1);
				Replaces_String(sh);
				Replaces_String(sc);
			
				if ((sh.find(sh1) != std::string::npos || sc.find(sc1) != std::string::npos) || 
					(sh1.find(sh) != std::string::npos || sc1.find(sc) != std::string::npos)) {
					sid = bet365[j]["results"][k]["id"].GetString();
					break;
				}
			
			}
			if (sid.length())
				break;
		}
		ndiff = atoi(sid.c_str());
		if (ndiff == 0)
			continue;
		match_inf *pmatch = new match_inf;
		memset(pmatch, 0, sizeof(match_inf));
		pmatch->hga_inx = i;
		pmatch->match_id = ndiff;
		//g_matches.push_back(pmatch);
		gs_strLastResponse = "";
		ndiff = GetBet365_Odd(ndiff);
		Document odds;
		if (ndiff == CURLE_OK) {
			odds.Parse(gs_strLastResponse.c_str());
			if (odds.HasMember("results") && odds["results"][0].HasMember("asian_lines")) {
				const Value& sp = odds["results"][0]["asian_lines"]["sp"];
				
				////////////////////////////////////////////////////////////////////////////////////////////////
				// start check for asian handicap
				////////////////////////////////////////////////////////////////////////////////////////////////
				string bodd1, bodd2, bhandicap1, bhandicap2;
				float bhandi1, bhandi2, hhandi1, hhandi2, bfodd1, bfodd2, hfodd1, hfodd2;

				if (sp.HasMember("asian_handicap")) {
					bodd1 = sp["asian_handicap"]["odds"][0]["odds"].GetString();
					bodd2 = sp["asian_handicap"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["asian_handicap"]["odds"][0]["handicap"].GetString();
					bhandicap2 = sp["asian_handicap"]["odds"][1]["handicap"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());

					hhandi1 = atof(shandicap.c_str());
					hhandi2 = -hhandi1;

					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());
					hfodd1 = atof(shodd.c_str());
					hfodd2 = atof(scodd.c_str());
					if (sstrong == "H") {
						hhandi1 = hhandi2;
						hhandi2 = -hhandi2;
					}
					//Check Arbitrage chance first
					if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
						h1 = checkArbitrage(bfodd1, hfodd2);
						h2 = checkArbitrage(hfodd1, bfodd2);
						pmatch->arinf[0].h1 = h1;
						pmatch->arinf[0].h2 = h2;
						pmatch->arinf[0].h1_odd1 = bfodd1;
						pmatch->arinf[0].h1_odd2 = hfodd2;
						pmatch->arinf[0].h2_odd1 = hfodd1;
						pmatch->arinf[0].h2_odd2 = bfodd2;
					}

					stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
					h1 = calc_middle(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2);
					pmatch->inf[0][pmatch->mcnt[0]].middle = h1;
					pmatch->inf[0][pmatch->mcnt[0]].handi1 = bhandi1;
					pmatch->inf[0][pmatch->mcnt[0]].handi2 = hhandi2;;
					pmatch->inf[0][pmatch->mcnt[0]].odd1 = bfodd1;
					pmatch->inf[0][pmatch->mcnt[0]++].odd2 = hfodd2;

					stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
					h2 = calc_middle(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2);
					pmatch->inf[0][pmatch->mcnt[0]].middle = h2;
					pmatch->inf[0][pmatch->mcnt[0]].handi1 = hhandi1;
					pmatch->inf[0][pmatch->mcnt[0]].handi2 = bhandi2;;
					pmatch->inf[0][pmatch->mcnt[0]].odd1 = hfodd1;
					pmatch->inf[0][pmatch->mcnt[0]++].odd2 = bfodd2;

					//User alternative lines to calc middling.
					if (odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_asian_handicap")) {
								const Value& aoddes = others[j]["sp"]["alternative_asian_handicap"]["odds"];
								nalt = aoddes.Size();
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									int ah = atoi(aheader.c_str());
									string ahandicap = aoddes[k]["handicap"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (ah == 1) {
										stake1 = 100; stake2 = stake1 * aodd / hfodd2;
										h1 = calc_middle(stake1, stake2, aodd, hfodd2, ahandi, hhandi2);
										pmatch->inf[0][pmatch->mcnt[0]].middle = h1;
										pmatch->inf[0][pmatch->mcnt[0]].handi1 = ahandi;
										pmatch->inf[0][pmatch->mcnt[0]].handi2 = hhandi2;;
										pmatch->inf[0][pmatch->mcnt[0]].odd1 = aodd;
										pmatch->inf[0][pmatch->mcnt[0]].nalt = 1;
										pmatch->inf[0][pmatch->mcnt[0]++].odd2 = hfodd2;
									}
									else {
										stake1 = 100; stake2 = stake1 * hfodd1 / aodd;
										h1 = calc_middle(stake1, stake2, hfodd1, aodd, hhandi1, ahandi);
										pmatch->inf[0][pmatch->mcnt[0]].middle = h1;
										pmatch->inf[0][pmatch->mcnt[0]].handi1 = hhandi1;
										pmatch->inf[0][pmatch->mcnt[0]].handi2 = ahandi;;
										pmatch->inf[0][pmatch->mcnt[0]].odd1 = hfodd1;
										pmatch->inf[0][pmatch->mcnt[0]].nalt = 1;
										pmatch->inf[0][pmatch->mcnt[0]++].odd2 = aodd;
									}
								}
								break;
							}
						}
					}
				}
				///////////////////////////////////////////////////////////////////////////////////////////////
				// Check For goal lines
				///////////////////////////////////////////////////////////////////////////////////////////////
				if (sp.HasMember("goal_line")) {
					bodd1 = sp["goal_line"]["odds"][0]["odds"].GetString();
					bodd2 = sp["goal_line"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["goal_line"]["odds"][0]["name"].GetString();
					bhandicap2 = sp["goal_line"]["odds"][1]["name"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());

					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());
					hfodd1 = atof(sgoaloodd.c_str());
					hfodd2 = atof(sgoaluodd.c_str());
					sgoalo = ReplaceAll(sgoalo, "O", "");
					sgoalu = ReplaceAll(sgoalu, "U", "");
					hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
					hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');
					if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
						h1 = checkArbitrage(bfodd1, hfodd2);
						h2 = checkArbitrage(hfodd1, bfodd2);
						pmatch->arinf[1].h1 = h1;
						pmatch->arinf[1].h2 = h2;
						pmatch->arinf[1].h1_odd1 = bfodd1;
						pmatch->arinf[1].h1_odd2 = hfodd2;
						pmatch->arinf[1].h2_odd1 = hfodd1;
						pmatch->arinf[1].h2_odd2 = bfodd2;
					}

					stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
					h1 = calc_middle_ou(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2, 'o', 'u');
					pmatch->inf[1][pmatch->mcnt[1]].middle = h1;
					pmatch->inf[1][pmatch->mcnt[1]].handi1 = bhandi1;
					pmatch->inf[1][pmatch->mcnt[1]].handi2 = hhandi2;;
					pmatch->inf[1][pmatch->mcnt[1]].odd1 = bfodd1;
					pmatch->inf[1][pmatch->mcnt[1]++].odd2 = hfodd2;

					stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
					h2 = calc_middle_ou(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2, 'o', 'u');
					pmatch->inf[1][pmatch->mcnt[1]].middle = h2;
					pmatch->inf[1][pmatch->mcnt[1]].handi1 = hhandi1;
					pmatch->inf[1][pmatch->mcnt[1]].handi2 = bhandi2;;
					pmatch->inf[1][pmatch->mcnt[1]].odd1 = hfodd1;
					pmatch->inf[1][pmatch->mcnt[1]++].odd2 = bfodd2;
					if (odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_goal_line")) {
								const Value& aoddes = others[j]["sp"]["alternative_goal_line"]["odds"];
								nalt = aoddes.Size();
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									string ahandicap = aoddes[k]["name"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (aheader == "Over") {
										stake1 = 100; stake2 = stake1 * aodd / hfodd2;
										h1 = calc_middle_ou(stake1, stake2, aodd, hfodd2, ahandi, hhandi2, 'o', 'u');
										pmatch->inf[1][pmatch->mcnt[1]].middle = h1;
										pmatch->inf[1][pmatch->mcnt[1]].handi1 = ahandi;
										pmatch->inf[1][pmatch->mcnt[1]].handi2 = hhandi2;;
										pmatch->inf[1][pmatch->mcnt[1]].odd1 = aodd;
										pmatch->inf[1][pmatch->mcnt[1]].odd2 = hfodd2;
										pmatch->inf[1][pmatch->mcnt[1]++].nalt = 1;
									}
									else {
										stake1 = 100; stake2 = stake1 * hfodd1 / aodd;
										h1 = calc_middle_ou(stake1, stake2, hfodd1, aodd, hhandi1, ahandi, 'o', 'u');
										pmatch->inf[1][pmatch->mcnt[1]].middle = h1;
										pmatch->inf[1][pmatch->mcnt[1]].handi1 = hhandi1;
										pmatch->inf[1][pmatch->mcnt[1]].handi2 = ahandi;;
										pmatch->inf[1][pmatch->mcnt[1]].odd1 = hfodd1;
										pmatch->inf[1][pmatch->mcnt[1]].odd2 = aodd;
										pmatch->inf[1][pmatch->mcnt[1]++].nalt = 1;
									}
								}
								break;
							}
						}
					}
				}
				//////////////////////////////////////////////////////////////////////////////////////////////////////////
				// 1st asian handicap
				//////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (sp.HasMember("1st_half_asian_handicap")) {
					bodd1 = sp["1st_half_asian_handicap"]["odds"][0]["odds"].GetString();
					bodd2 = sp["1st_half_asian_handicap"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["1st_half_asian_handicap"]["odds"][0]["handicap"].GetString();
					bhandicap2 = sp["1st_half_asian_handicap"]["odds"][1]["handicap"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());

					hhandi1 = atof(s1sthandicap.c_str());
					hhandi2 = -hhandi1;

					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());
					hfodd1 = atof(s1sthodd.c_str());
					hfodd2 = atof(s1stcodd.c_str());
					if (sstrong == "H") {
						hhandi1 = hhandi2;
						hhandi2 = -hhandi2;
					}
					if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
						h1 = checkArbitrage(bfodd1, hfodd2);
						h2 = checkArbitrage(hfodd1, bfodd2);
						pmatch->arinf[2].h1 = h1;
						pmatch->arinf[2].h2 = h2;
						pmatch->arinf[2].h1_odd1 = bfodd1;
						pmatch->arinf[2].h1_odd2 = hfodd2;
						pmatch->arinf[2].h2_odd1 = hfodd1;
						pmatch->arinf[2].h2_odd2 = bfodd2;
					}

					//Check Middling
					stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
					h1 = calc_middle(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2);
					pmatch->inf[2][pmatch->mcnt[2]].middle = h1;
					pmatch->inf[2][pmatch->mcnt[2]].handi1 = bhandi1;
					pmatch->inf[2][pmatch->mcnt[2]].handi2 = hhandi2;;
					pmatch->inf[2][pmatch->mcnt[2]].odd1 = bfodd1;
					pmatch->inf[2][pmatch->mcnt[2]++].odd2 = hfodd2;

					stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
					h2 = calc_middle(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2);
					pmatch->inf[2][pmatch->mcnt[2]].middle = h2;
					pmatch->inf[2][pmatch->mcnt[2]].handi1 = hhandi1;
					pmatch->inf[2][pmatch->mcnt[2]].handi2 = bhandi2;;
					pmatch->inf[2][pmatch->mcnt[2]].odd1 = hfodd1;
					pmatch->inf[2][pmatch->mcnt[2]++].odd2 = bfodd2;

					//User alternative lines to calc middling.
					if (odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_1st_half_asian_handicap")) {
								const Value& aoddes = others[j]["sp"]["alternative_1st_half_asian_handicap"]["odds"];
								nalt = aoddes.Size();
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									int ah = atoi(aheader.c_str());
									string ahandicap = aoddes[k]["handicap"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (ah == 1) {
										stake1 = 100; stake2 = stake1 * aodd / hfodd2;
										h1 = calc_middle(stake1, stake2, aodd, hfodd2, ahandi, hhandi2);
										pmatch->inf[2][pmatch->mcnt[2]].middle = h1;
										pmatch->inf[2][pmatch->mcnt[2]].handi1 = ahandi;
										pmatch->inf[2][pmatch->mcnt[2]].handi2 = hhandi2;;
										pmatch->inf[2][pmatch->mcnt[2]].odd1 = aodd;
										pmatch->inf[2][pmatch->mcnt[2]].nalt = 1;
										pmatch->inf[2][pmatch->mcnt[2]++].odd2 = hfodd2;
									}
									else {
										stake1 = 100; stake2 = stake1 * hfodd1 / aodd;
										h1 = calc_middle(stake1, stake2, hfodd1, aodd, hhandi1, ahandi);
										pmatch->inf[2][pmatch->mcnt[2]].middle = h1;
										pmatch->inf[2][pmatch->mcnt[2]].handi1 = hhandi1;
										pmatch->inf[2][pmatch->mcnt[2]].handi2 = ahandi;;
										pmatch->inf[2][pmatch->mcnt[2]].odd1 = hfodd1;
										pmatch->inf[2][pmatch->mcnt[2]].nalt = 1;
										pmatch->inf[2][pmatch->mcnt[2]++].odd2 = aodd;
									}
								}
								break;
							}
						}
					}
				}
				//////////////////////////////////////////////////////////////////////////////////
				//	1st asian goal line
				/////////////////////////////////////////////////////////////////////////////////
				if (sp.HasMember("1st_half_goal_line")) {
					bodd1 = sp["1st_half_goal_line"]["odds"][0]["odds"].GetString();
					bodd2 = sp["1st_half_goal_line"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["1st_half_goal_line"]["odds"][0]["name"].GetString();
					bhandicap2 = sp["1st_half_goal_line"]["odds"][1]["name"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());

					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());
					hfodd1 = atof(s1stgoaloodd.c_str());
					hfodd2 = atof(s1stgoaluodd.c_str());
					s1stgoalo = ReplaceAll(s1stgoalo, "O", "");
					s1stgoalu = ReplaceAll(s1stgoalu, "U", "");
					hhandi1 = get_handicap_from_string(s1stgoalo.c_str(), '/');
					hhandi2 = get_handicap_from_string(s1stgoalu.c_str(), '/');
					if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
						h1 = checkArbitrage(bfodd1, hfodd2);
						h2 = checkArbitrage(hfodd1, bfodd2);
						pmatch->arinf[3].h1 = h1;
						pmatch->arinf[3].h2 = h2;
						pmatch->arinf[3].h1_odd1 = bfodd1;
						pmatch->arinf[3].h1_odd2 = hfodd2;
						pmatch->arinf[3].h2_odd1 = hfodd1;
						pmatch->arinf[3].h2_odd2 = bfodd2;
					}

					stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
					h1 = calc_middle_ou(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2, 'o', 'u');
					pmatch->inf[3][pmatch->mcnt[3]].middle = h1;
					pmatch->inf[3][pmatch->mcnt[3]].handi1 = bhandi1;
					pmatch->inf[3][pmatch->mcnt[3]].handi2 = hhandi2;;
					pmatch->inf[3][pmatch->mcnt[3]].odd1 = bfodd1;
					pmatch->inf[3][pmatch->mcnt[3]++].odd2 = hfodd2;

					stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
					h2 = calc_middle_ou(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2, 'o', 'u');
					pmatch->inf[3][pmatch->mcnt[3]].middle = h2;
					pmatch->inf[3][pmatch->mcnt[3]].handi1 = hhandi1;
					pmatch->inf[3][pmatch->mcnt[3]].handi2 = bhandi2;;
					pmatch->inf[3][pmatch->mcnt[3]].odd1 = hfodd1;
					pmatch->inf[3][pmatch->mcnt[3]++].odd2 = bfodd2;
					if (odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_1st_half_goal_line")) {
								const Value& aoddes = others[j]["sp"]["alternative_1st_half_goal_line"]["odds"];
								nalt = aoddes.Size();
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									string ahandicap = aoddes[k]["name"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (aheader == "Over") {
										stake1 = 100; stake2 = stake1 * aodd / hfodd2;
										h1 = calc_middle_ou(stake1, stake2, aodd, hfodd2, ahandi, hhandi2, 'o', 'u');
										pmatch->inf[3][pmatch->mcnt[3]].middle = h1;
										pmatch->inf[3][pmatch->mcnt[3]].handi1 = ahandi;
										pmatch->inf[3][pmatch->mcnt[3]].handi2 = hhandi2;;
										pmatch->inf[3][pmatch->mcnt[3]].odd1 = aodd;
										pmatch->inf[3][pmatch->mcnt[3]].odd2 = hfodd2;
										pmatch->inf[3][pmatch->mcnt[3]++].nalt = 1;
									}
									else {
										stake1 = 100; stake2 = stake1 * hfodd1 / aodd;
										h1 = calc_middle_ou(stake1, stake2, hfodd1, aodd, hhandi1, ahandi, 'o', 'u');
										pmatch->inf[3][pmatch->mcnt[3]].middle = h1;
										pmatch->inf[3][pmatch->mcnt[3]].handi1 = hhandi1;
										pmatch->inf[3][pmatch->mcnt[3]].handi2 = ahandi;;
										pmatch->inf[3][pmatch->mcnt[3]].odd1 = hfodd1;
										pmatch->inf[3][pmatch->mcnt[3]].odd2 = aodd;
										pmatch->inf[3][pmatch->mcnt[3]++].nalt = 1;
									}
								}
								break;
							}
						}
					}
				}
				//////////////////////////////////////////////////////////////////////////////////
				// asian_handicap_corners
				//////////////////////////////////////////////////////////////////////////////////

				if (hga[i].HasMember("CN_DATA")) {
					const Value& cn_data = hga[i]["CN_DATA"];
					shandicap = cn_data["RATIO_R"].IsNull() ? "null" : cn_data["RATIO_R"].GetString();
					shodd = cn_data["IOR_RH"].IsNull() ? "null" : cn_data["IOR_RH"].GetString();
					scodd = cn_data["IOR_RC"].IsNull() ? "null" : cn_data["IOR_RC"].GetString();
					sgoalo = cn_data["RATIO_OUO"].IsNull() ? "null" : cn_data["RATIO_OUO"].GetString();
					sgoalu = cn_data["RATIO_OUU"].IsNull() ? "null" : cn_data["RATIO_OUU"].GetString();
					sgoaloodd = cn_data["IOR_OUH"].IsNull() ? "null" : cn_data["IOR_OUH"].GetString();
					sgoaluodd = cn_data["IOR_OUC"].IsNull() ? "null" : cn_data["IOR_OUC"].GetString();
					if (sp.HasMember("asian_handicap_corners")) {
						bodd1 = sp["asian_handicap_corners"]["odds"][0]["odds"].GetString();
						bodd2 = sp["asian_handicap_corners"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["asian_handicap_corners"]["odds"][0]["handicap"].GetString();
						bhandicap2 = sp["asian_handicap_corners"]["odds"][1]["handicap"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						hhandi1 = atof(shandicap.c_str());
						hhandi2 = -hhandi1;

						bfodd1 = atof(bodd1.c_str());
						bfodd2 = atof(bodd2.c_str());
						hfodd1 = atof(shodd.c_str());
						hfodd2 = atof(scodd.c_str());
						if (sstrong == "H") {
							hhandi1 = hhandi2;
							hhandi2 = -hhandi2;
						}
						//Check Arbitrage chance first
						if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							h2 = checkArbitrage(hfodd1, bfodd2);
							pmatch->arinf[4].h1 = h1;
							pmatch->arinf[4].h2 = h2;
							pmatch->arinf[4].h1_odd1 = bfodd1;
							pmatch->arinf[4].h1_odd2 = hfodd2;
							pmatch->arinf[4].h2_odd1 = hfodd1;
							pmatch->arinf[4].h2_odd2 = bfodd2;
						}

						stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
						h1 = calc_middle(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2);
						pmatch->inf[4][pmatch->mcnt[4]].middle = h1;
						pmatch->inf[4][pmatch->mcnt[4]].handi1 = bhandi1;
						pmatch->inf[4][pmatch->mcnt[4]].handi2 = hhandi2;;
						pmatch->inf[4][pmatch->mcnt[4]].odd1 = bfodd1;
						pmatch->inf[4][pmatch->mcnt[4]++].odd2 = hfodd2;

						stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
						h2 = calc_middle(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2);
						pmatch->inf[4][pmatch->mcnt[4]].middle = h2;
						pmatch->inf[4][pmatch->mcnt[4]].handi1 = hhandi1;
						pmatch->inf[4][pmatch->mcnt[4]].handi2 = bhandi2;;
						pmatch->inf[4][pmatch->mcnt[4]].odd1 = hfodd1;
						pmatch->inf[4][pmatch->mcnt[4]++].odd2 = bfodd2;
					}
					/////////////////////////////////////////////////////////////
					// asian_total_corners
					/////////////////////////////////////////////////////////////
					if (sp.HasMember("asian_total_corners")) {
						bodd1 = sp["asian_total_corners"]["odds"][0]["odds"].GetString();
						bodd2 = sp["asian_total_corners"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["asian_total_corners"]["odds"][0]["name"].GetString();
						bhandicap2 = sp["asian_total_corners"]["odds"][1]["name"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						bfodd1 = atof(bodd1.c_str());
						bfodd2 = atof(bodd2.c_str());
						hfodd1 = atof(sgoaloodd.c_str());
						hfodd2 = atof(sgoaluodd.c_str());
						sgoalo = ReplaceAll(sgoalo, "O", "");
						sgoalu = ReplaceAll(sgoalu, "U", "");
						hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
						hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');
						if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							h2 = checkArbitrage(hfodd1, bfodd2);
							pmatch->arinf[5].h1 = h1;
							pmatch->arinf[5].h2 = h2;
							pmatch->arinf[5].h1_odd1 = bfodd1;
							pmatch->arinf[5].h1_odd2 = hfodd2;
							pmatch->arinf[5].h2_odd1 = hfodd1;
							pmatch->arinf[5].h2_odd2 = bfodd2;
						}

						stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
						h1 = calc_middle_ou(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2, 'o', 'u');
						pmatch->inf[5][pmatch->mcnt[5]].middle = h1;
						pmatch->inf[5][pmatch->mcnt[5]].handi1 = bhandi1;
						pmatch->inf[5][pmatch->mcnt[5]].handi2 = hhandi2;;
						pmatch->inf[5][pmatch->mcnt[5]].odd1 = bfodd1;
						pmatch->inf[5][pmatch->mcnt[5]++].odd2 = hfodd2;

						stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
						h2 = calc_middle_ou(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2, 'o', 'u');
						pmatch->inf[5][pmatch->mcnt[5]].middle = h2;
						pmatch->inf[5][pmatch->mcnt[5]].handi1 = hhandi1;
						pmatch->inf[5][pmatch->mcnt[5]].handi2 = bhandi2;;
						pmatch->inf[5][pmatch->mcnt[5]].odd1 = hfodd1;
						pmatch->inf[5][pmatch->mcnt[5]++].odd2 = bfodd2;
					}
					////////////////////////////////////////////////////////////////
					// 1st_half_asian_corners
					///////////////////////////////////////////////////////////////
					if (sp.HasMember("asian_total_corners")) {
						sgoalo = cn_data["RATIO_HOUO"].IsNull() ? "null" : cn_data["RATIO_HOUO"].GetString();
						sgoalu = cn_data["RATIO_HOUU"].IsNull() ? "null" : cn_data["RATIO_HOUU"].GetString();
						sgoaloodd = cn_data["IOR_HOUH"].IsNull() ? "null" : cn_data["IOR_HOUH"].GetString();
						sgoaluodd = cn_data["IOR_HOUC"].IsNull() ? "null" : cn_data["IOR_HOUC"].GetString();

						bodd1 = sp["1st_half_asian_corners"]["odds"][0]["odds"].GetString();
						bodd2 = sp["1st_half_asian_corners"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["1st_half_asian_corners"]["odds"][0]["name"].GetString();
						bhandicap2 = sp["1st_half_asian_corners"]["odds"][1]["name"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						bfodd1 = atof(bodd1.c_str());
						bfodd2 = atof(bodd2.c_str());
						hfodd1 = atof(sgoaloodd.c_str());
						hfodd2 = atof(sgoaluodd.c_str());
						sgoalo = ReplaceAll(sgoalo, "O", "");
						sgoalu = ReplaceAll(sgoalu, "U", "");
						hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
						hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');
						if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							h2 = checkArbitrage(hfodd1, bfodd2);
							pmatch->arinf[6].h1 = h1;
							pmatch->arinf[6].h2 = h2;
							pmatch->arinf[6].h1_odd1 = bfodd1;
							pmatch->arinf[6].h1_odd2 = hfodd2;
							pmatch->arinf[6].h2_odd1 = hfodd1;
							pmatch->arinf[6].h2_odd2 = bfodd2;
						}

						stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
						h1 = calc_middle_ou(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2, 'o', 'u');
						pmatch->inf[6][pmatch->mcnt[6]].middle = h1;
						pmatch->inf[6][pmatch->mcnt[6]].handi1 = bhandi1;
						pmatch->inf[6][pmatch->mcnt[6]].handi2 = hhandi2;;
						pmatch->inf[6][pmatch->mcnt[6]].odd1 = bfodd1;
						pmatch->inf[6][pmatch->mcnt[6]++].odd2 = hfodd2;

						stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
						h2 = calc_middle_ou(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2, 'o', 'u');
						pmatch->inf[6][pmatch->mcnt[6]].middle = h2;
						pmatch->inf[6][pmatch->mcnt[6]].handi1 = hhandi1;
						pmatch->inf[6][pmatch->mcnt[6]].handi2 = bhandi2;;
						pmatch->inf[6][pmatch->mcnt[6]].odd1 = hfodd1;
						pmatch->inf[6][pmatch->mcnt[6]++].odd2 = bfodd2;
					}
				}

				//////////////////////////////////////////////////////////////////////////////
				//  Asian handicap cards
				/////////////////////////////////////////////////////////////////////////////
				if (hga[i].HasMember("RN_DATA")) {
					const Value& rn_data = hga[i]["RN_DATA"];
					shandicap = rn_data["RATIO_R"].IsNull() ? "null" : rn_data["RATIO_R"].GetString();
					shodd = rn_data["IOR_RH"].IsNull() ? "null" : rn_data["IOR_RH"].GetString();
					scodd = rn_data["IOR_RC"].IsNull() ? "null" : rn_data["IOR_RC"].GetString();
					if (sp.HasMember("asian_handicap_cards")) {
						bodd1 = sp["asian_handicap_cards"]["odds"][0]["odds"].GetString();
						bodd2 = sp["asian_handicap_cards"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["asian_handicap_cards"]["odds"][0]["handicap"].GetString();
						bhandicap2 = sp["asian_handicap_cards"]["odds"][1]["handicap"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						hhandi1 = atof(shandicap.c_str());
						hhandi2 = -hhandi1;

						bfodd1 = atof(bodd1.c_str());
						bfodd2 = atof(bodd2.c_str());
						hfodd1 = atof(shodd.c_str());
						hfodd2 = atof(scodd.c_str());
						if (sstrong == "H") {
							hhandi1 = hhandi2;
							hhandi2 = -hhandi2;
						}
						//Check Arbitrage chance first
						if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							h2 = checkArbitrage(hfodd1, bfodd2);
							pmatch->arinf[7].h1 = h1;
							pmatch->arinf[7].h2 = h2;
							pmatch->arinf[7].h1_odd1 = bfodd1;
							pmatch->arinf[7].h1_odd2 = hfodd2;
							pmatch->arinf[7].h2_odd1 = hfodd1;
							pmatch->arinf[7].h2_odd2 = bfodd2;
						}

						//Check Middling
						stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
						h1 = calc_middle(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2);
						pmatch->inf[7][pmatch->mcnt[7]].middle = h1;
						pmatch->inf[7][pmatch->mcnt[7]].handi1 = bhandi1;
						pmatch->inf[7][pmatch->mcnt[7]].handi2 = hhandi2;;
						pmatch->inf[7][pmatch->mcnt[7]].odd1 = bfodd1;
						pmatch->inf[7][pmatch->mcnt[7]++].odd2 = hfodd2;

						stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
						h2 = calc_middle(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2);
						pmatch->inf[7][pmatch->mcnt[7]].middle = h2;
						pmatch->inf[7][pmatch->mcnt[7]].handi1 = hhandi1;
						pmatch->inf[7][pmatch->mcnt[7]].handi2 = bhandi2;;
						pmatch->inf[7][pmatch->mcnt[7]].odd1 = hfodd1;
						pmatch->inf[7][pmatch->mcnt[7]++].odd2 = bfodd2;

					}
					////////////////////////////////////////////////////////////////////////////////
					// asian_total_cards
					////////////////////////////////////////////////////////////////////////////////

					sgoalo = rn_data["RATIO_OUO"].IsNull() ? "null" : rn_data["RATIO_OUO"].GetString();
					sgoalu = rn_data["RATIO_OUU"].IsNull() ? "null" : rn_data["RATIO_OUU"].GetString();
					sgoaloodd = rn_data["IOR_OUH"].IsNull() ? "null" : rn_data["IOR_OUH"].GetString();
					sgoaluodd = rn_data["IOR_OUC"].IsNull() ? "null" : rn_data["IOR_OUC"].GetString();

					if (sp.HasMember("asian_total_cards")) {
						bodd1 = sp["asian_total_cards"]["odds"][0]["odds"].GetString();
						bodd2 = sp["asian_total_cards"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["asian_total_cards"]["odds"][0]["name"].GetString();
						bhandicap2 = sp["asian_total_cards"]["odds"][1]["name"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						bfodd1 = atof(bodd1.c_str());
						bfodd2 = atof(bodd2.c_str());
						hfodd1 = atof(sgoaloodd.c_str());
						hfodd2 = atof(sgoaluodd.c_str());
						sgoalo = ReplaceAll(sgoalo, "O", "");
						sgoalu = ReplaceAll(sgoalu, "U", "");
						hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
						hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');
						if (fabs(hhandi1 - bhandi1) < 0.0001 && fabs(hhandi2 - bhandi2) < 0.0001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							h2 = checkArbitrage(hfodd1, bfodd2);
							pmatch->arinf[8].h1 = h1;
							pmatch->arinf[8].h2 = h2;
							pmatch->arinf[8].h1_odd1 = bfodd1;
							pmatch->arinf[8].h1_odd2 = hfodd2;
							pmatch->arinf[8].h2_odd1 = hfodd1;
							pmatch->arinf[8].h2_odd2 = bfodd2;
						}

						stake1 = 100; stake2 = stake1 * bfodd1 / hfodd2;
						h1 = calc_middle_ou(stake1, stake2, bfodd1, hfodd2, bhandi1, hhandi2, 'o', 'u');
						pmatch->inf[8][pmatch->mcnt[8]].middle = h1;
						pmatch->inf[8][pmatch->mcnt[8]].handi1 = bhandi1;
						pmatch->inf[8][pmatch->mcnt[8]].handi2 = hhandi2;;
						pmatch->inf[8][pmatch->mcnt[8]].odd1 = bfodd1;
						pmatch->inf[8][pmatch->mcnt[8]++].odd2 = hfodd2;

						stake1 = 100; stake2 = stake1 * hfodd1 / bfodd2;
						h2 = calc_middle_ou(stake1, stake2, hfodd1, bfodd2, hhandi1, bhandi2, 'o', 'u');
						pmatch->inf[8][pmatch->mcnt[8]].middle = h2;
						pmatch->inf[8][pmatch->mcnt[8]].handi1 = hhandi1;
						pmatch->inf[8][pmatch->mcnt[8]].handi2 = bhandi2;;
						pmatch->inf[8][pmatch->mcnt[8]].odd1 = hfodd1;
						pmatch->inf[8][pmatch->mcnt[8]++].odd2 = bfodd2;
					}
				}
			}
		}
		g_matches.push_back(pmatch);
		m_show.SetPos(i);
	}
	g_updated = 1;
	int nend_time = GetTickCount();
	memset(buff, 0, sizeof(wchar_t) * 200);
	swprintf(buff, 200, L"%d seconds. %d matches found.", (nend_time - nstart_time) / 1000, g_matches.size());
	SetDlgItemText(IDC_STATIC_COUNT, buff);
}
int COppCheckDlg::GetBet365_Odd(int match_id) {
	char buff[260] = { 0 };
	sprintf(buff, "https://betsapi2.p.rapidapi.com/v3/bet365/prematch?FI=%d", match_id);
	CURL *hnd = curl_easy_init();

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(hnd, CURLOPT_URL, buff);

	struct curl_slist *headers = NULL;
	curl_easy_setopt(hnd, CURLOPT_CAINFO, "cacert.pem");
	headers = curl_slist_append(headers, "X-RapidAPI-Key: e2fa07ad89msh0857ce6a64b3678p13de20jsn4cdcdb04c8ff");
	headers = curl_slist_append(headers, "X-RapidAPI-Host: betsapi2.p.rapidapi.com");
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, function_pt);        // set a callback to capture the server's response
	curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);

	CURLcode ret = curl_easy_perform(hnd);
	curl_slist_free_all(headers);
	curl_easy_cleanup(hnd);
	return ret;
}
void COppCheckDlg::getWStr(const char *timestamp, wchar_t *wstr) {
	memset(wstr, 0, 200 * sizeof(wchar_t));
	const size_t cSize = strlen(timestamp) + 1;
	mbstowcs(wstr, timestamp, cSize);
}
void COppCheckDlg::CalculateResult()
{

}
void COppCheckDlg::getWhere(int n, wchar_t *p) {
	switch (n)
	{
	case 0:
		wcscpy(p, L"Asian Handicap");
		break;
	case 1:
		wcscpy(p, L"Goal Line");
		break;
	case 2:
		wcscpy(p, L"1st Half Asian Handicap");
		break;
	case 3:
		wcscpy(p, L"1st Half Goal Line");
		break;
	case 4:
		wcscpy(p, L"Asian Total Corners");
		break;
	case 5:
		wcscpy(p, L"Asian Handicap Corners");
		break;
	case 6:
		wcscpy(p, L"1st Half Asian Corners");
		break;
	case 7:
		wcscpy(p, L"Asian Total Cards");
		break;
	case 8:
		wcscpy(p, L"Asian Handicap Cards");
		break;
	default:
		break;
	}
}
DWORD COppCheckDlg::display_func() {
	int filelen1, ns;
	FILE *fhga;
	char *readBuffer1;
	wchar_t buff[260] = { 0 };
	int i, idx, j, narb, k, nmid;
	float middle;

	while (1)
	{
		while (1) {
			if (g_updated == 1)
				break;
			Sleep(1000);
		}
		m_org.SetRedraw(FALSE);
		m_org.DeleteAllItems();
		m_org.SetRedraw(TRUE);

		fhga = fopen("hga/hga038-data.json", "rb"); // non-Windows use "r"
		if (fhga == NULL)
			return 0;
		fseek(fhga, 0, SEEK_END);
		filelen1 = ftell(fhga);
		fseek(fhga, 0, SEEK_SET);
		readBuffer1 = new char[filelen1 + 1];
		memset(readBuffer1, 0, filelen1 + 1);
		fread(readBuffer1, 1, filelen1, fhga);
		fclose(fhga);

		Document d;
		d.Parse(readBuffer1);
		delete readBuffer1;

		ns = g_matches.size();
		idx = 0;
		
		for (i = 0; i < ns; i++) {
			match_inf *pm = g_matches.at(i);
			string sdatetime = d[pm->hga_inx]["DATETIME"].GetString();
			string sleague = d[pm->hga_inx]["LEAGUE"].GetString();
			string steamh = d[pm->hga_inx]["TEAM_H"].GetString();
			string steamc = d[pm->hga_inx]["TEAM_C"].GetString();

			TimeStamp2String(sdatetime.c_str(), buff);
			m_org.InsertItem(idx, buff);

			getWStr(sleague.c_str(), buff);
			m_org.SetItemText(idx, 1, buff);

			getWStr(steamh.c_str(), buff);
			m_org.SetItemText(idx, 2, buff);

			getWStr(steamc.c_str(), buff);
			m_org.SetItemText(idx, 3, buff);
			narb = 0; nmid = 0;
			for (j = 0; j < CHECK_PARAMS; j++) {
				if (pm->arinf[j].h1 > 0) {
					if (narb) {
						m_org.InsertItem(idx + narb, L"");
					}
					memset(buff, 0, 260);
					swprintf(buff, 260, L"%.3f", pm->arinf[j].h1);
					m_org.SetItemText(idx + narb, 4, buff);

					memset(buff, 0, 260);
					getWhere(j, buff);
					m_org.SetItemText(idx + narb, 5, buff);

					memset(buff, 0, 260);
					swprintf(buff, 260, L"%.3f", pm->arinf[j].h1_odd1);
					m_org.SetItemText(idx + narb, 6, buff);

					memset(buff, 0, 260);
					swprintf(buff, 260, L"%.3f", pm->arinf[j].h1_odd2);
					m_org.SetItemText(idx + narb, 7, buff);
					narb++;
				}
				if (pm->arinf[j].h2 > 0) {
					if (narb) {
						m_org.InsertItem(idx + narb, L"");
					}
					memset(buff, 0, 260);
					swprintf(buff, 260, L"%.3f", pm->arinf[j].h2);
					m_org.SetItemText(idx + narb, 4, buff);

					memset(buff, 0, 260);
					getWhere(j, buff);
					m_org.SetItemText(idx + narb, 5, buff);

					memset(buff, 0, 260);
					swprintf(buff, 260, L"%.3f", pm->arinf[j].h2_odd1);
					m_org.SetItemText(idx + narb, 6, buff);

					memset(buff, 0, 260);
					swprintf(buff, 260, L"%.3f", pm->arinf[j].h2_odd2);
					m_org.SetItemText(idx + narb, 7, buff);
					narb++;
				}
			}
			for (j = 0; j < CHECK_PARAMS; j++) {
				for (k = 0; k < pm->mcnt[j]; k++) {
					middle = pm->inf[j][k].middle;
					if (middle > 4.0) {
						if (nmid == 0) {
							if (narb == 0)
								narb = 1;

							m_org.InsertItem(idx + narb, L"");
							narb++;

							m_org.InsertItem(idx + narb, L"");
							m_org.SetItemText(idx + narb, 1, L"Middle");
							m_org.SetItemText(idx + narb, 2, L"");
							m_org.SetItemText(idx + narb, 3, L"Handicap1");
							m_org.SetItemText(idx + narb, 4, L"Handicap2");
							m_org.SetItemText(idx + narb, 5, L"Where");
							m_org.SetItemText(idx + narb, 6, L"Odd1");
							m_org.SetItemText(idx + narb, 7, L"Odd2");

							nmid++;
						}
						m_org.InsertItem(idx + narb + nmid, L"");

		
						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f", middle);
						m_org.SetItemText(idx + narb + nmid, 1, buff);

						if (pm->inf[j][k].nalt == 1)
							m_org.SetItemText(idx + narb + nmid, 2, L"AlternativeLine");						

						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f", pm->inf[j][k].handi1);
						m_org.SetItemText(idx + narb + nmid, 3, buff);

						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f", pm->inf[j][k].handi2);
						m_org.SetItemText(idx + narb + nmid, 4, buff);

						memset(buff, 0, 260);
						getWhere(j, buff);
						m_org.SetItemText(idx + narb + nmid, 5, buff);

						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f", pm->inf[j][k].odd1);
						m_org.SetItemText(idx + narb + nmid, 6, buff);

						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f", pm->inf[j][k].odd2);
						m_org.SetItemText(idx + narb + nmid, 7, buff);

						nmid++;
					}
				}
			}
			if (narb + nmid > 0) {
				idx += narb;
				idx += nmid;
				m_org.InsertItem(idx++, L"");
				m_org.InsertItem(idx++, L"");
			}
			else {
				idx++;
				m_org.InsertItem(idx++, L"");
				m_org.InsertItem(idx++, L"");
			}
		}
		g_updated = 0;
		SetWindowText(L"Done.");
	}
	return 0;
}
DWORD COppCheckDlg::display_thread(LPVOID param) {
	COppCheckDlg *pDlg = (COppCheckDlg*)param;
	if (pDlg == NULL)
		return 0;
	return pDlg->display_func();
}
DWORD COppCheckDlg::catch_func() {
	while (1) {
		m_status = 0;
		gs_strLastResponse = "";
		SetWindowText(L"Downloading HGA038's data. please wait...");
		GetHGA();
		m_status = 1;
		SetWindowText(L"Downloading Bet365's upcoming data. please wait...");
		GetBet365_Upcoming();
		m_status = 2;
		SetWindowText(L"Downloading Bet365's odd data and calculating. please wait...");
		ReadDataDisplay();//HGA centraltime zone, GMT time zone.

		SetWindowText(L"Almost done. please wait...");
		m_status = 3;

		Sleep(300 * 1000); //Read Every 5 min
	}
}
DWORD COppCheckDlg::catch_thread(LPVOID param) {
	COppCheckDlg *pDlg = (COppCheckDlg*)param;
	if (pDlg == NULL)
		return 0;
	return pDlg->catch_func();
}
void COppCheckDlg::TimeStamp2String(const char *timestamp, wchar_t *strtime) {
	time_t tmer = atoi(timestamp);
	tm *tt = localtime(&tmer);
	memset(strtime, 0, 200 * sizeof(wchar_t));
	wsprintf(strtime, L"%04d-%02d-%02d %02d:%02d:%02d", tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec);
}
void COppCheckDlg::OnBnClickedButton1()
{

}
