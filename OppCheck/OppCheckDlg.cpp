
// OppCheckDlg.cpp : implementation file
//

#include "stdafx.h"
#include "stdlib.h"
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

//#define TEST_MODE
#define PLAYER_WIN	 0
#define LOSE		 1
#define STAKE_REFUND 2
#define HALF_WIN	 3
#define HALF_LOSE	 4
#define MIN_ARBIT   100.6
typedef struct state_struct {
	int goal_diff;
	int state1;
	int state2;
} STATE_STRUCT;

string gs_betresponse;
string gs_strLastResponse;
vector<match_inf *> g_matches;
int g_updated = 0;
int g_nFilter = 5;
string g_textFilter;
match_inf *pinterested_match = NULL;
match_inf *new_int_match = NULL;
int m_autorefresh = 0;
int goal_filter = 0;
int corner_filter = 0;
int book_filter = 0;

int isSameString(string str1, string str2) {
	if (str1 == str2)
		return 1;
	int n1 = str1.find(0x20);
	int n2 = str2.find(0x20);
	if (n1 == 0 && n2 == 0)
		return 0;
	string str2_1 = str2 + " ";
	string str2_2 = " " + str2;
	string str1_1 = str1 + " ";
	string str1_2 = " "+str1;
	if (str1.find(str2_1) != string::npos)
		return 1;
	if (str1.find(str2_2) != string::npos)
		return 1;

	if (str2.find(str1_1) != string::npos)
		return 1;
	if (str2.find(str1_2) != string::npos)
		return 1;
	return 0;
}

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
	m_hcatch_auto = INVALID_HANDLE_VALUE;
	m_hcatch = INVALID_HANDLE_VALUE;
}
COppCheckDlg::~COppCheckDlg() {
	clear_match();
	int j;
	if (new_int_match != NULL) {
		delete new_int_match->arinf;
		for (j = 0; j < CHECK_PARAMS; j++)
			delete new_int_match->inf[j];
		delete new_int_match;
	}
}
void COppCheckDlg::OnOK() {

}
void COppCheckDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LST_ORG, m_org);
	DDX_Control(pDX, IDC_PROGRESS1, m_show);
	DDX_Control(pDX, IDC_EDIT_FILTER, m_filteredit);
	DDX_Control(pDX, IDC_LIST1, m_lstint);
	DDX_Control(pDX, IDC_CHK_AUTO, m_autochk);
	DDX_Control(pDX, IDC_REFRESH, m_btn_refresh);
	DDX_Control(pDX, IDC_COMBO1, m_seltype);
}

BEGIN_MESSAGE_MAP(COppCheckDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &COppCheckDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BTN_ADDTO, &COppCheckDlg::OnBnClickedBtnAddto)
	ON_BN_CLICKED(IDC_CHK_AUTO, &COppCheckDlg::OnBnClickedChkAuto)
	ON_BN_CLICKED(IDC_REFRESH, &COppCheckDlg::OnBnClickedRefresh)
	ON_BN_CLICKED(IDC_CHK_GOAL, &COppCheckDlg::OnBnClickedChkGoal)
	ON_BN_CLICKED(IDC_CHK_CORNER, &COppCheckDlg::OnBnClickedChkCorner)
	ON_BN_CLICKED(IDC_CHK_BOOK, &COppCheckDlg::OnBnClickedChkBook)
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
	//m_hcatch = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)catch_thread, this, 0, &dwThreadId);
	//m_hdisplay = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)display_thread, this, 0, &dwThreadId);
#ifndef TEST_MODE
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)inter_thread, this, 0, &dwThreadId);
#endif
	m_hdisplay = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)display_thread, this, 0, &dwThreadId);
	
	Initlist();
	
	SetDlgItemInt(IDC_EDIT_FILTER, 5);
	CFont myFont;
	myFont.CreateFont(16, 0, 0, 0, FW_HEAVY, true, false,
		0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FIXED_PITCH | FF_MODERN, _T("Courier New"));

	m_filteredit.SetFont(&myFont);

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
	int i, idx = 0, state1, state2, ct, ct1;
	int nv1 = (int)(handicap1 * 100);
	int nv2 = (int)(handicap2 * 100);
	struct state_struct stt_array[10];
	float values[10] = { 0 };
	float tmp,favg,tmp1;


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
				tmp1 = values[i] / fabs(favg)+1;
				tmp+= tmp1;
			}
		}
		return tmp;
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
	int i,idx = 0,ct,ct1;
	int state1, state2;
	float tmp, favg, tmp1;
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
				tmp1 = values[i] / fabs(favg)+1;
				tmp += tmp1;
			}
		}
		return tmp;
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
	//return xx * odd2 - x;
	float x1 = 1.0 / odd1;
	float y1 = 1.0 / odd2;
	xx = (x1 + y1) * 100;
	return xx;
	//return 1.0 - (x1 + y1);
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
	m_lstint.InsertColumn(0, L"DATE");
	m_lstint.SetColumnWidth(0, 120);

	m_org.InsertColumn(1, L"League");
	m_org.SetColumnWidth(1, 120);
	m_lstint.InsertColumn(0, L"DATE");
	m_lstint.SetColumnWidth(0, 120);

	m_org.InsertColumn(2, L"Team(Home)");
	m_org.SetColumnWidth(2, 120);
	m_lstint.InsertColumn(2, L"Team(Home)");
	m_lstint.SetColumnWidth(2, 120);

	m_org.InsertColumn(3, L"Team(Away)");
	m_org.SetColumnWidth(3, 120);
	m_lstint.InsertColumn(3, L"Team(Away)");
	m_lstint.SetColumnWidth(3, 120);

	m_org.InsertColumn(4, L"Arbitrage");
	m_org.SetColumnWidth(4, 80);
	m_lstint.InsertColumn(4, L"Arbitrage");
	m_lstint.SetColumnWidth(4, 80);

	m_org.InsertColumn(5, L"Where");
	m_org.SetColumnWidth(5, 130);
	m_lstint.InsertColumn(5, L"Where");
	m_lstint.SetColumnWidth(5, 130);

	m_org.InsertColumn(6, L"Odd1");
	m_org.SetColumnWidth(6, 80);
	m_lstint.InsertColumn(6, L"Odd1");
	m_lstint.SetColumnWidth(6, 80);

	m_org.InsertColumn(7, L"Odd2");
	m_org.SetColumnWidth(7, 100);
	m_lstint.InsertColumn(7, L"Odd2");
	m_lstint.SetColumnWidth(7, 100);

	m_org.InsertColumn(8, L"Identifier");
	m_org.SetColumnWidth(8, 80);

	m_org.InsertColumn(9, L"BID");
	m_org.SetColumnWidth(9, 80);

	m_seltype.AddString(L"Search by name");
	m_seltype.AddString(L"Search by middle");
	m_seltype.SetCurSel(1);
	m_lstint.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);
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
size_t function_pt1(void *ptr, size_t size, size_t nmemb, void * /*stream*/)
{
	gs_betresponse += (const char*)ptr;
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
	string a[23] = { "AC ", "AFC ", "BSC ", "BK ", "CA ", "CD ", "CF ", "CR ", "CS ", "CSD ", "FC ", "IA ", "RC ", "SA ", "SC ", "SV ", "JF ", "GD ", "Club ", "Women " };
	string b[24] = { " AC", " AFC", " BSC", " BK", " CA", " CD", " CF", " CR", " CS", " CSD", " FC", " IA", " RC", " SA", " SC", " SV", " JF", " GD", " Club", " (W)", " Women" };
	for (i = 0; i < 20; i++) {
		len = a[i].length();
		if (len < str.length()) {
			tmp = str.substr(0, len);
			if (tmp == a[i])
				str = ReplaceAll(str, a[i], "");
		}
	}
	for (i = 0; i < 21; i++) {
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
	int i, j;
	for (i = 0; i < size; i++) {
		match_inf *pinf = g_matches.at(i);
		delete pinf->arinf;
		for (j = 0; j < CHECK_PARAMS; j++)
			delete pinf->inf[j];

		delete pinf;		
	}	
	g_matches.clear();
	pinterested_match = NULL;
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
void COppCheckDlg::Asian_GoalLine(match_inf *pmatch, float *bhandi1, float *bhandi2, float *hhandi1, float *hhandi2,
	float *bfodd1, float *bfodd2, float *hfodd1, float *hfodd2, int inx) {
	int hcount = 0, bcount = 0;
	int i, j;
	float stake1, stake2;
	float mid, ar;
	for (i = 0; i < 10; i++) {
		if (fabs(hfodd1[i]) < 1.001) {
			hcount = i;
			break;
		}
	}
	hcount = (hcount == 0) ? 10 : hcount;

	for (i = 0; i < 20; i++) {
		if (fabs(bfodd1[i]) < 1.001) {
			bcount = i;
			break;
		}
	}
	bcount = (bcount == 0) ? 20 : bcount;

	for (i = 0; i < hcount; i++) {
		for (j = 0; j < hcount; j++) {
			if (fabs(hhandi1[i]-hhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(hfodd1[i], hfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = hfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = hfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'H';
					pmatch->arinf[pmatch->arcnt++].c2 = 'H';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				//calc middling.
				if (hhandi2[j] < hhandi1[i])
					continue;
				stake1 = 1000; stake2 = hfodd1[i] * stake1 / hfodd2[j];
				mid = calc_middle_ou(stake1, stake2, hfodd1[i], hfodd2[j], hhandi1[i], hhandi2[j], 'o','u');
				pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
				pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = hhandi1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = hhandi2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = hfodd1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = hfodd2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 3;
				pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'H';
				pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'H';
				
				pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
			}
		}
		for (j = 0; j < bcount; j++) {
			if (fabs(hhandi1[i]-bhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(hfodd1[i], bfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = hfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = bfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'H';
					pmatch->arinf[pmatch->arcnt++].c2 = 'B';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				if (bhandi2[j] < hhandi1[i])
					continue; //can be polish middling
				stake1 = 1000; stake2 = hfodd1[i] * stake1 / bfodd2[j];
				mid = calc_middle_ou(stake1, stake2, hfodd1[i], bfodd2[j], hhandi1[i], bhandi2[j],'o','u');
				pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
				pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = hhandi1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = bhandi2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = hfodd1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = bfodd2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 3;
				pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'H';
				pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'B';
				pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
			}
		}
	}
	for (i = 0; i < bcount; i++) {
		for (j = 0; j < bcount; j++) {
			if (fabs(bhandi1[i] -bhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(bfodd1[i], bfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = bfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = bfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'B';
					pmatch->arinf[pmatch->arcnt++].c2 = 'B';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				if (bhandi2[j] < bhandi1[i])
					continue; //can be polish middling
				stake1 = 1000; stake2 = bfodd1[i] * stake1 / bfodd2[j];
				mid = calc_middle_ou(stake1, stake2, bfodd1[i], bfodd2[j], bhandi1[i], bhandi2[j],'o','u');
				pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
				pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = bhandi1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = bhandi2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = bfodd1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = bfodd2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 3;
				pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'B';
				pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'B';
				pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
			}
		}
		for (j = 0; j < hcount; j++) {
			if (fabs(bhandi1[i]-hhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(bfodd1[i], hfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = bfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = hfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'B';
					pmatch->arinf[pmatch->arcnt++].c2 = 'H';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				if (hhandi2[j] < bhandi1[i])
					continue; //can be polish middling
				stake1 = 1000; stake2 = bfodd1[i] * stake1 / hfodd2[j];
				mid = calc_middle_ou(stake1, stake2, bfodd1[i], hfodd2[j], bhandi1[i], hhandi2[j],'o','u');
				pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
				pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = bhandi1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = hhandi2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = bfodd1[i];
				pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = hfodd2[j];
				pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 3;
				pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'B';
				pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'H';				
				pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
			}
		}
	}

}
void COppCheckDlg::Asian_Handicap(match_inf *pmatch, float *bhandi1, float *bhandi2, float *hhandi1, float *hhandi2,
	float *bfodd1, float *bfodd2, float *hfodd1, float *hfodd2, int inx) {
	//Check Arbitrage chance first
	int hcount = 0, bcount = 0;
	int i, j;
	float stake1, stake2;
	float mid, ar;
	for (i = 0; i < 10; i++) {
		if (fabs(hfodd1[i]) < 1.001) {
			hcount = i;
			break;
		}
	}
	hcount = (hcount == 0)? 10 : hcount;

	for (i = 0; i < 20; i++) {
		if (fabs(bfodd1[i]) < 1.001) {
			bcount = i;
			break;
		}
	}
	bcount = (bcount == 0) ? 20 : bcount;

	for (i = 0; i < bcount; i++) {
		for (j = 0; j < bcount; j++) {
			if (fabs(bhandi1[i] + bhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(bfodd1[i], bfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = bfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = bfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'B';
					pmatch->arinf[pmatch->arcnt++].c2 = 'B';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				//calc middling.
				if (bhandi1[i] >= 0) {
					if (fabs(bhandi2[j]) >= bhandi1[i] || bhandi2[j] > 0)
						continue; //can be polish middling
					stake1 = 1000; stake2 = bfodd1[i] * stake1 / bfodd2[j];
					mid = calc_middle(stake1, stake2, bfodd1[i], bfodd2[j], bhandi1[i], bhandi2[j]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = bhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = bhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = bfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = bfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'B';

					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'B';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
				else {
					if (bhandi2[j] <= fabs(bhandi1[i]) || bhandi2[j] < 0)
						continue; //can be polish middling
					stake2 = 1000; stake1 = bfodd2[j] * stake2 / bfodd1[i];
					mid = calc_middle(stake2, stake1, bfodd2[j], bfodd1[i], bhandi2[j], bhandi1[i]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = bhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = bhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = bfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = bfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'B';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'B';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
			}
		}
		for (j = 0; j < hcount; j++) {
			if (fabs(bhandi1[i] + hhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(bfodd1[i], hfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = bfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = hfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'B';
					pmatch->arinf[pmatch->arcnt++].c2 = 'H';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				//calc middling.
				if (bhandi1[i] >= 0) {
					if (fabs(hhandi2[j]) >= bhandi1[i] || hhandi2[j] > 0)
						continue; //can be polish middling
					stake1 = 1000; stake2 = bfodd1[i] * stake1 / hfodd2[j];
					mid = calc_middle(stake1, stake2, bfodd1[i], hfodd2[j], bhandi1[i], hhandi2[j]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = bhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = hhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = bfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = hfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'B';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'H';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
				else {
					if (hhandi2[j] <= fabs(bhandi1[i]) || hhandi2[j] < 0)
						continue; //can be polish middling
					stake2 = 1000; stake1 = hfodd2[j] * stake2 / bfodd1[i];
					mid = calc_middle(stake2, stake1, hfodd2[j], bfodd1[i], hhandi2[j], bhandi1[i]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = hhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = bhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = hfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = bfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'H';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'B';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
			}
		}
	}

	for (i = 0; i < hcount; i++) {
		for (j = 0; j < hcount; j++) {
			if (fabs(hhandi1[i] + hhandi2[j]) < 0.001) {
				//calc abitrage
				ar=checkArbitrage(hfodd1[i], hfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = hfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = hfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'H';
					pmatch->arinf[pmatch->arcnt++].c2 = 'H';
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				//calc middling.
				if (hhandi1[i] >= 0) {
					if (fabs(hhandi2[j]) >= hhandi1[i] || hhandi2[j] > 0)
						continue; //can be polish middling
					stake1 = 1000; stake2 = hfodd1[i] * stake1 / hfodd2[j];
					mid = calc_middle(stake1, stake2, hfodd1[i], hfodd2[j], hhandi1[i], hhandi2[j]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = hhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = hhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = hfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = hfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'H';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'H';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
				else {
					if (hhandi2[j] <= fabs(hhandi1[i]) || hhandi2[j] < 0)
						continue; //can be polish middling
					stake2 = 1000; stake1 = hfodd2[j] * stake2 / hfodd1[i];
					mid = calc_middle(stake2, stake1, hfodd2[j], hfodd1[i], hhandi2[j], hhandi1[i]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = hhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = hhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = hfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = hfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'H';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'H';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
			}
		}
		for (j = 0; j < bcount; j++) {
			if (fabs(hhandi1[i]+bhandi2[j]) < 0.001) {
				//calc abitrage
				ar = checkArbitrage(hfodd1[i], bfodd2[j]);
				if (ar <= MIN_ARBIT) {
					pmatch->arinf[pmatch->arcnt].ar = ar;
					pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1[i]);
					pmatch->arinf[pmatch->arcnt].odd1 = hfodd1[i];
					pmatch->arinf[pmatch->arcnt].odd2 = bfodd2[j];
					pmatch->arinf[pmatch->arcnt].nwhere = inx;
					pmatch->arinf[pmatch->arcnt].c1 = 'H';
					pmatch->arinf[pmatch->arcnt++].c2 = 'B';					
					pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
				}
			}
			else {
				//calc middling.
				if (hhandi1[i] >= 0) {
					if (fabs(bhandi2[j]) >= hhandi1[i] || bhandi2[j] > 0)
						continue; //can be polish middling
					stake1 = 1000; stake2 = hfodd1[i] * stake1 / bfodd2[j];
					mid = calc_middle(stake1, stake2, hfodd1[i], bfodd2[j], hhandi1[i], bhandi2[j]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = hhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = bhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = hfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = bfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'H';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'B';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
				else {
					if (bhandi2[j] <= fabs(hhandi1[i]) || bhandi2[j] < 0)
						continue; //can be polish middling
					stake2 = 1000; stake1 = bfodd2[j] * stake2 / hfodd1[i];
					mid = calc_middle(stake2, stake1, bfodd2[j], hfodd1[i], bhandi2[j], hhandi1[i]);
					pmatch->inf[inx][pmatch->mcnt[inx]].middle = mid;
					pmatch->inf[inx][pmatch->mcnt[inx]].handi1 = bhandi2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].handi2 = hhandi1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd1 = bfodd2[j];
					pmatch->inf[inx][pmatch->mcnt[inx]].odd2 = hfodd1[i];
					pmatch->inf[inx][pmatch->mcnt[inx]].nalt = 1;
					pmatch->inf[inx][pmatch->mcnt[inx]].c1 = 'B';
					pmatch->inf[inx][pmatch->mcnt[inx]++].c2 = 'H';
					pmatch->inf[inx] = (middle_inf*)realloc(pmatch->inf[inx], (pmatch->mcnt[inx] + 1) * sizeof(middle_inf));
				}
			}
		}
	}
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
	float h1, stake1, stake2;
	wchar_t buff[200] = { 0 };
	int idx = 0, nalt, commtime = 0, ntimestart, ntimeend;
	g_updated = 0;
	clear_match();

	int nstart_time = GetTickCount();
	m_show.SetRange(0, nhga - 1);
	for (i = 0; i < nhga; i++) {
		m_show.SetPos(i);
		string hid = hga[i]["ID"].GetString();
		string sdatetime = hga[i]["DATETIME"].GetString();
		string showtype = hga[i]["showtype"].GetString();
		string lid = hga[i]["uid"].GetString();
		string sleaguename = hga[i]["LEAGUE"].IsNull() ? "null" : hga[i]["LEAGUE"].GetString();
		if (sleaguename.find("Fantasy Matches") != string::npos) {
			continue;
		}

		string shometeam = hga[i]["TEAM_H"].IsNull() ? "null" : hga[i]["TEAM_H"].GetString();
		string sawayteam = hga[i]["TEAM_C"].IsNull() ? "null" : hga[i]["TEAM_C"].GetString();		
		string sstrong, shandicap, shodd, scodd, sgoalo, sgoalu, sgoaloodd, sgoaluodd;	
		int handi_alt = hga[i]["handicap_count"].GetInt();
		int half_alt = hga[i]["1sthalf_count"].GetInt();
		if (handi_alt == 0 || half_alt == 0)
			continue;

		string sid = "";
		int ndiff = 0;
		for (j = 0; j < nbet; j++) {
			if (!bet365[j].HasMember("results"))
				continue;
			nk = bet365[j]["results"].Size();
			for (k = 0; k < nk; k++) {
				string stime = bet365[j]["results"][k]["time"].GetString();
				ndiff = abs(atoi(stime.c_str()) - atoi(sdatetime.c_str()));
				if (ndiff > 1200)
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
				
				if (isSameString(sh, sh1) || isSameString(sc, sc1)) {
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
		pmatch->arcnt = 0;
		pmatch->pthis = pmatch;
		pmatch->bid = ndiff;
		pmatch->hid = atoi(hid.c_str());
		pmatch->lid = atoi(lid.c_str());
		strcpy(pmatch->showtype, showtype.c_str());

		pmatch->arinf = new arbit_inf;
		for (j = 0;j < CHECK_PARAMS; j++)
			pmatch->inf[j] = new middle_inf;

		//g_matches.push_back(pmatch);
		gs_strLastResponse = "";
		ntimestart = GetTickCount();
		ndiff = GetBet365_Odd(ndiff);
		ntimeend = GetTickCount();
		commtime += ntimeend - ntimestart;
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

				float h_handis1[10] = { 0 };
				float h_handis2[10] = { 0 };
				float h_odds1[10] = { 0 };
				float h_odds2[10] = { 0 };
				float b_handis1[20] = { 0 };
				float b_handis2[20] = { 0 };
				float b_odds1[20] = { 0 };
				float b_odds2[20] = { 0 };

				char alt_buf[20] = { 0 };
				int idx1, idx2;
				float v1, v2, o1, o2;

				if (sp.HasMember("asian_handicap")) {
					bodd1 = sp["asian_handicap"]["odds"][0]["odds"].GetString();
					bodd2 = sp["asian_handicap"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["asian_handicap"]["odds"][0]["handicap"].GetString();
					bhandicap2 = sp["asian_handicap"]["odds"][1]["handicap"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());
					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());
					b_handis1[0] = bhandi1;
					b_handis2[0] = bhandi2;
					b_odds1[0] = bfodd1;
					b_odds2[0] = bfodd2;

					for (j = 0;j < handi_alt; j++) {
						sprintf(alt_buf, "handicap_%d", j+1);
						const Value &altline = hga[i][alt_buf];
						sstrong = altline["STRONG"].GetString();
						shandicap = altline["RATIO_R"].IsNull() ? "null" : altline["RATIO_R"].GetString();
						shodd = altline["IOR_RH"].IsNull() ? "null" : altline["IOR_RH"].GetString();
						scodd = altline["IOR_RC"].IsNull() ? "null" : altline["IOR_RC"].GetString();
						hfodd1 = atof(shodd.c_str());
						hfodd2 = atof(scodd.c_str());
						hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
						hhandi2 = -hhandi1;
						if (sstrong == "H") {
							hhandi1 = hhandi2;
							hhandi2 = -hhandi2;
						}
						h_handis1[j] = hhandi1;
						h_handis2[j] = hhandi2;
						h_odds1[j] = hfodd1;
						h_odds2[j] = hfodd2;
					}
					if (sp.HasMember("alternative_asian_handicap") && odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_asian_handicap")) {
								const Value& aoddes = others[j]["sp"]["alternative_asian_handicap"]["odds"];
								nalt = aoddes.Size();
								idx1 = 1; idx2 = 1;
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									int ah = atoi(aheader.c_str());
									string ahandicap = aoddes[k]["handicap"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (ah == 1) {
										b_handis1[idx1] = ahandi;
										b_odds1[idx1] = aodd;
										idx1++;
									}
									else if (ah == 2) {
										b_handis2[idx2] = ahandi;
										b_odds2[idx2] = aodd;
										idx2++;
									}
								}
								break;
							}
						}
					}
					Asian_Handicap(pmatch, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2,0);
				}
				///////////////////////////////////////////////////////////////////////////////////////////////
				// Check For goal lines
				///////////////////////////////////////////////////////////////////////////////////////////////
				if (sp.HasMember("goal_line")) {
					memset(h_handis1, 0, 4*sizeof(float));
					memset(h_handis2,0,4 * sizeof(float));
					memset(h_odds1,0,4 * sizeof(float));
					memset(h_odds2,0,4 * sizeof(float));
					memset(b_handis1,0, 20 * sizeof(float));
					memset(b_handis2,0, 20 * sizeof(float));
					memset(b_odds1,0, 20 * sizeof(float));
					memset(b_odds2,0, 20 * sizeof(float));

					bodd1 = sp["goal_line"]["odds"][0]["odds"].GetString();
					bodd2 = sp["goal_line"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["goal_line"]["odds"][0]["name"].GetString();
					bhandicap2 = sp["goal_line"]["odds"][1]["name"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());
					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());					
					
					b_handis1[0] = bhandi1;
					b_handis2[0] = bhandi2;
					b_odds1[0] = bfodd1;
					b_odds2[0] = bfodd2;

					memset(alt_buf, 0, 20);
					for (j = 0;j < handi_alt; j++) {
						sprintf(alt_buf, "handicap_%d", j+1);
						const Value &altline = hga[i][alt_buf];
						
						sgoalo = altline["RATIO_OUO"].IsNull() ? "null" : altline["RATIO_OUO"].GetString();
						sgoalu = altline["RATIO_OUU"].IsNull() ? "null" : altline["RATIO_OUU"].GetString();
						sgoaloodd = altline["IOR_OUH"].IsNull() ? "null" : altline["IOR_OUH"].GetString();
						sgoaluodd = altline["IOR_OUC"].IsNull() ? "null" : altline["IOR_OUC"].GetString();

						hfodd1 = atof(sgoaloodd.c_str());
						hfodd2 = atof(sgoaluodd.c_str());
						sgoalo = ReplaceAll(sgoalo, "O", "");
						sgoalu = ReplaceAll(sgoalu, "U", "");
						hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
						hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');

						h_handis1[j] = hhandi1;
						h_handis2[j] = hhandi2;
						h_odds1[j] = hfodd1;
						h_odds2[j] = hfodd2;
					}
					if (sp.HasMember("alternative_goal_line") && odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_goal_line")) {
								const Value& aoddes = others[j]["sp"]["alternative_goal_line"]["odds"];
								nalt = aoddes.Size();
								idx1 = 1; idx2 = 1;
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									string ahandicap = aoddes[k]["name"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (aheader == "Over") {
										b_handis1[idx1] = ahandi;
										b_odds1[idx1] = aodd;
										idx1++;
									}
									else {
										b_handis2[idx2] = ahandi;
										b_odds2[idx2] = aodd;
										idx2++;
									}
								}
								break;
							}
						}
					}
					Asian_GoalLine(pmatch, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2,1);
				}
				//////////////////////////////////////////////////////////////////////////////////////////////////////////
				// 1st asian handicap
				//////////////////////////////////////////////////////////////////////////////////////////////////////////
				if (sp.HasMember("1st_half_asian_handicap")) {
					memset(h_handis1, 0, 4 * sizeof(float));
					memset(h_handis2, 0, 4 * sizeof(float));
					memset(h_odds1, 0, 4 * sizeof(float));
					memset(h_odds2, 0, 4 * sizeof(float));
					memset(b_handis1, 0, 20 * sizeof(float));
					memset(b_handis2, 0, 20 * sizeof(float));
					memset(b_odds1, 0, 20 * sizeof(float));
					memset(b_odds2, 0, 20 * sizeof(float));
					bodd1 = sp["1st_half_asian_handicap"]["odds"][0]["odds"].GetString();
					bodd2 = sp["1st_half_asian_handicap"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["1st_half_asian_handicap"]["odds"][0]["handicap"].GetString();
					bhandicap2 = sp["1st_half_asian_handicap"]["odds"][1]["handicap"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());
					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());

					b_handis1[0] = bhandi1;
					b_handis2[0] = bhandi2;
					b_odds1[0] = bfodd1;
					b_odds2[0] = bfodd2;
					memset(alt_buf, 0, 20);
					for (j = 0;j < half_alt; j++) {
						sprintf(alt_buf, "1sthalf_%d", j+1);
						const Value &altline = hga[i][alt_buf];
						sstrong = altline["STRONG"].GetString();
						shandicap = altline["RATIO_HR"].IsNull() ? "null" : altline["RATIO_HR"].GetString();
						shodd = altline["IOR_HRH"].IsNull() ? "null" : altline["IOR_HRH"].GetString();
						scodd = altline["IOR_HRC"].IsNull() ? "null" : altline["IOR_HRC"].GetString();
						hfodd1 = atof(shodd.c_str());
						hfodd2 = atof(scodd.c_str());
						hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
						hhandi2 = -hhandi1;
						if (sstrong == "H") {
							hhandi1 = hhandi2;
							hhandi2 = -hhandi2;
						}
						h_handis1[j] = hhandi1;
						h_handis2[j] = hhandi2;
						h_odds1[j] = hfodd1;
						h_odds2[j] = hfodd2;
					}
					//User alternative lines to calc middling.
					if (sp.HasMember("alternative_1st_half_asian_handicap") && odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_1st_half_asian_handicap")) {
								const Value& aoddes = others[j]["sp"]["alternative_1st_half_asian_handicap"]["odds"];
								nalt = aoddes.Size();
								idx1 = 1; idx2 = 1;
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									int ah = atoi(aheader.c_str());
									string ahandicap = aoddes[k]["handicap"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (ah == 1) {
										b_handis1[idx1] = ahandi;
										b_odds1[idx1] = aodd;
										idx1++;
									}
									else if (ah == 2) {
										b_handis2[idx2] = ahandi;
										b_odds2[idx2] = aodd;
										idx2++;
									}
								}
								break;
							}
						}
					}
					Asian_Handicap(pmatch, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2, 2);
				}
				//////////////////////////////////////////////////////////////////////////////////
				//	1st asian goal line
				/////////////////////////////////////////////////////////////////////////////////
				if (sp.HasMember("1st_half_goal_line")) {
					memset(h_handis1, 0, 4 * sizeof(float));
					memset(h_handis2, 0, 4 * sizeof(float));
					memset(h_odds1, 0, 4 * sizeof(float));
					memset(h_odds2, 0, 4 * sizeof(float));
					memset(b_handis1, 0, 20 * sizeof(float));
					memset(b_handis2, 0, 20 * sizeof(float));
					memset(b_odds1, 0, 20 * sizeof(float));
					memset(b_odds2, 0, 20 * sizeof(float));

					bodd1 = sp["1st_half_goal_line"]["odds"][0]["odds"].GetString();
					bodd2 = sp["1st_half_goal_line"]["odds"][1]["odds"].GetString();
					bhandicap1 = sp["1st_half_goal_line"]["odds"][0]["name"].GetString();
					bhandicap2 = sp["1st_half_goal_line"]["odds"][1]["name"].GetString();
					bhandi1 = get_handicap_from_string(bhandicap1.c_str());
					bhandi2 = get_handicap_from_string(bhandicap2.c_str());

					bfodd1 = atof(bodd1.c_str());
					bfodd2 = atof(bodd2.c_str());
					
					b_handis1[0] = bhandi1;
					b_handis2[0] = bhandi2;
					b_odds1[0] = bfodd1;
					b_odds2[0] = bfodd2;

					memset(alt_buf, 0, 20);

					for (j = 0;j < half_alt; j++) {
						sprintf(alt_buf, "1sthalf_%d", j+1);
						const Value &altline = hga[i][alt_buf];

						sgoalo = altline["RATIO_HOUO"].IsNull() ? "null" : altline["RATIO_HOUO"].GetString();
						sgoalu = altline["RATIO_HOUU"].IsNull() ? "null" : altline["RATIO_HOUU"].GetString();
						sgoaloodd = altline["IOR_HOUH"].IsNull() ? "null" : altline["IOR_HOUH"].GetString();
						sgoaluodd = altline["IOR_HOUC"].IsNull() ? "null" : altline["IOR_HOUC"].GetString();

						hfodd1 = atof(sgoaloodd.c_str());
						hfodd2 = atof(sgoaluodd.c_str());
						sgoalo = ReplaceAll(sgoalo, "O", "");
						sgoalu = ReplaceAll(sgoalu, "U", "");
						hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
						hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');

						h_handis1[j] = hhandi1;
						h_handis2[j] = hhandi2;
						h_odds1[j] = hfodd1;
						h_odds2[j] = hfodd2;
					}

					if (sp.HasMember("alternative_1st_half_asian_handicap") && odds["results"][0].HasMember("others")) {
						const Value &others = odds["results"][0]["others"];
						ndiff = others.Size();
						for (j = 0; j < ndiff; j++) {
							if (others[j]["sp"].HasMember("alternative_1st_half_goal_line")) {
								const Value& aoddes = others[j]["sp"]["alternative_1st_half_goal_line"]["odds"];
								nalt = aoddes.Size();
								idx1 = 1;idx2 = 2;
								for (k = 0; k < nalt; k++) {
									string saodd = aoddes[k]["odds"].GetString();
									float aodd = atof(saodd.c_str());
									string aheader = aoddes[k]["header"].GetString();
									string ahandicap = aoddes[k]["name"].GetString();
									float ahandi = get_handicap_from_string(ahandicap.c_str());
									if (aheader == "Over") {
										b_handis1[idx1] = ahandi;
										b_odds1[idx1] = aodd;
										idx1++;
									}
									else {
										b_handis2[idx2] = ahandi;
										b_odds2[idx2] = aodd;
										idx2++;
									}
								}
								break;
							}
						}
					}
					
					Asian_GoalLine(pmatch, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2, 3);
				}
				//////////////////////////////////////////////////////////////////////////////////
				// asian_handicap_corners
				//////////////////////////////////////////////////////////////////////////////////

				if (hga[i].HasMember("CN_DATA")) {
					const Value& cn_data = hga[i]["CN_DATA"];
					sstrong = cn_data["STRONG"].IsNull() ? "null" : cn_data["STRONG"].GetString();
					shandicap = cn_data["RATIO_R"].IsNull() ? "null" : cn_data["RATIO_R"].GetString();
					if (shandicap != "null") {
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

							hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
							hhandi2 = -hhandi1;

							bfodd1 = atof(bodd1.c_str());
							bfodd2 = atof(bodd2.c_str());
							hfodd1 = atof(shodd.c_str());
							hfodd2 = atof(scodd.c_str());
							if (sstrong == "H") {
								hhandi1 = hhandi2;
								hhandi2 = -hhandi2;
							}
							if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
								h1 = checkArbitrage(hfodd1, bfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = hfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = bfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 4;
									pmatch->arinf[pmatch->arcnt].c1 = 'H';
									pmatch->arinf[pmatch->arcnt++].c2 = 'B';
						
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
								h1 = checkArbitrage(bfodd1, hfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = bfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = hfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 4;
									pmatch->arinf[pmatch->arcnt].c1 = 'B';
									pmatch->arinf[pmatch->arcnt++].c2 = 'H';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
								if (fabs(bhandi1) - fabs(hhandi1) < 0) {
									//This means hga offer high value.
									if (hhandi1 > 0) {
										v1 = hhandi1;
										o1 = hfodd1;
										v2 = bhandi2;
										o2 = bfodd2;
										pmatch->inf[4][pmatch->mcnt[4]].c1 = 'H';
										pmatch->inf[4][pmatch->mcnt[4]].c2 = 'B';
									}
									else {
										v1 = hhandi2;
										o1 = hfodd2;
										v2 = bhandi1;
										o2 = bfodd1;
										pmatch->inf[4][pmatch->mcnt[4]].c1 = 'H';
										pmatch->inf[4][pmatch->mcnt[4]].c2 = 'B';
									}
								}
								else {
									if (bhandi1 > 0) {
										v1 = bhandi1;
										o1 = bfodd1;
										v2 = hhandi2;
										o2 = hfodd2;
										pmatch->inf[4][pmatch->mcnt[4]].c1 = 'B';
										pmatch->inf[4][pmatch->mcnt[4]].c2 = 'H';
									}
									else {
										v1 = bhandi2;
										o1 = bfodd2;
										v2 = hhandi1;
										o2 = hfodd1;
										pmatch->inf[4][pmatch->mcnt[4]].c1 = 'B';
										pmatch->inf[4][pmatch->mcnt[4]].c2 = 'H';
									}
								}
								stake1 = 100; stake2 = stake1 * o1 / o2;
								h1 = calc_middle(stake1, stake2, o1, o2, v1, v2);
								pmatch->inf[4][pmatch->mcnt[4]].middle = h1;
								pmatch->inf[4][pmatch->mcnt[4]].handi1 = v1;
								pmatch->inf[4][pmatch->mcnt[4]].handi2 = v2;;
								pmatch->inf[4][pmatch->mcnt[4]].odd1 = o1;
								pmatch->inf[4][pmatch->mcnt[4]++].odd2 = o2;
								pmatch->inf[4] = (middle_inf*)realloc(pmatch->inf[4], (pmatch->mcnt[4] + 1) * sizeof(middle_inf));
							}
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

							if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
								h1 = checkArbitrage(hfodd1, bfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = hfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = bfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 5;
									pmatch->arinf[pmatch->arcnt].c1 = 'H';
									pmatch->arinf[pmatch->arcnt++].c2 = 'B';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
								h1 = checkArbitrage(bfodd1, hfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = bfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = hfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 5;
									pmatch->arinf[pmatch->arcnt].c1 = 'B';
									pmatch->arinf[pmatch->arcnt++].c2 = 'H';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (!((fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) || (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001))) {
								if (bhandi1 < hhandi2) {
									v1 = bhandi1;
									o1 = bfodd1;
									v2 = hhandi2;
									o2 = hfodd2;
									pmatch->inf[5][pmatch->mcnt[5]].c1 = 'B';
									pmatch->inf[5][pmatch->mcnt[5]].c2 = 'H';
								}
								else {
									v1 = hhandi1;
									o1 = hfodd1;
									v2 = bhandi2;
									o2 = bfodd2;
									pmatch->inf[5][pmatch->mcnt[5]].c1 = 'H';
									pmatch->inf[5][pmatch->mcnt[5]].c2 = 'B';
								}
								stake1 = 100; stake2 = stake1 * o1 / o2;
								h1 = calc_middle_ou(stake1, stake2, o1, o2, v1, v2, 'o', 'u');

								pmatch->inf[5][pmatch->mcnt[5]].middle = h1;
								pmatch->inf[5][pmatch->mcnt[5]].handi1 = v1;
								pmatch->inf[5][pmatch->mcnt[5]].handi2 = v2;;
								pmatch->inf[5][pmatch->mcnt[5]].odd1 = o1;
								pmatch->inf[5][pmatch->mcnt[5]].nalt = 2;
								pmatch->inf[5][pmatch->mcnt[5]++].odd2 = o2;
								pmatch->inf[5] = (middle_inf*)realloc(pmatch->inf[5], (pmatch->mcnt[5] + 1) * sizeof(middle_inf));
							}
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
							if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
								h1 = checkArbitrage(hfodd1, bfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = hfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = bfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 6;
									pmatch->arinf[pmatch->arcnt].c1 = 'H';
									pmatch->arinf[pmatch->arcnt++].c2 = 'B';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
								h1 = checkArbitrage(bfodd1, hfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = bfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = hfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 6;
									pmatch->arinf[pmatch->arcnt].c1 = 'B';
									pmatch->arinf[pmatch->arcnt++].c2 = 'H';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
								if (bhandi1 < hhandi2) {
									v1 = bhandi1;
									o1 = bfodd1;
									v2 = hhandi2;
									o2 = hfodd2;
									pmatch->inf[6][pmatch->mcnt[6]].c1 = 'B';
									pmatch->inf[6][pmatch->mcnt[6]].c2 = 'H';
								}
								else {
									v1 = hhandi1;
									o1 = hfodd1;
									v2 = bhandi2;
									o2 = bfodd2;
									pmatch->inf[6][pmatch->mcnt[6]].c1 = 'H';
									pmatch->inf[6][pmatch->mcnt[6]].c2 = 'B';
								}
								stake1 = 100; stake2 = stake1 * o1 / o2;
								h1 = calc_middle_ou(stake1, stake2, o1, o2, v1, v2, 'o', 'u');
								pmatch->inf[6][pmatch->mcnt[6]].middle = h1;
								pmatch->inf[6][pmatch->mcnt[6]].handi1 = v1;
								pmatch->inf[6][pmatch->mcnt[6]].handi2 = v2;;
								pmatch->inf[6][pmatch->mcnt[6]].odd1 = o1;
								pmatch->inf[6][pmatch->mcnt[6]].nalt = 2;
								pmatch->inf[6][pmatch->mcnt[6]++].odd2 = o2;
								pmatch->inf[6] = (middle_inf*)realloc(pmatch->inf[6], (pmatch->mcnt[6] + 1) * sizeof(middle_inf));
							}
						}
					}
				}

				//////////////////////////////////////////////////////////////////////////////
				//  Asian handicap cards
				/////////////////////////////////////////////////////////////////////////////
				if (hga[i].HasMember("RN_DATA")) {
					const Value& rn_data = hga[i]["RN_DATA"];
					sstrong = rn_data["STRONG"].IsNull() ? "null" : rn_data["STRONG"].GetString();
					shandicap = rn_data["RATIO_R"].IsNull() ? "null" : rn_data["RATIO_R"].GetString();
					if (shandicap != "null") {
						shodd = rn_data["IOR_RH"].IsNull() ? "null" : rn_data["IOR_RH"].GetString();
						scodd = rn_data["IOR_RC"].IsNull() ? "null" : rn_data["IOR_RC"].GetString();
						sgoalo = rn_data["RATIO_OUO"].IsNull() ? "null" : rn_data["RATIO_OUO"].GetString();
						sgoalu = rn_data["RATIO_OUU"].IsNull() ? "null" : rn_data["RATIO_OUU"].GetString();
						sgoaloodd = rn_data["IOR_OUH"].IsNull() ? "null" : rn_data["IOR_OUH"].GetString();
						sgoaluodd = rn_data["IOR_OUC"].IsNull() ? "null" : rn_data["IOR_OUC"].GetString();

						if (sp.HasMember("asian_handicap_cards")) {
							bodd1 = sp["asian_handicap_cards"]["odds"][0]["odds"].GetString();
							bodd2 = sp["asian_handicap_cards"]["odds"][1]["odds"].GetString();
							bhandicap1 = sp["asian_handicap_cards"]["odds"][0]["handicap"].GetString();
							bhandicap2 = sp["asian_handicap_cards"]["odds"][1]["handicap"].GetString();
							bhandi1 = get_handicap_from_string(bhandicap1.c_str());
							bhandi2 = get_handicap_from_string(bhandicap2.c_str());

							hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
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
							if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
								h1 = checkArbitrage(hfodd1, bfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = hfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = bfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 7;
									pmatch->arinf[pmatch->arcnt].c1 = 'H';
									pmatch->arinf[pmatch->arcnt++].c2 = 'B';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
								h1 = checkArbitrage(bfodd1, hfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = bfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = hfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 7;
									pmatch->arinf[pmatch->arcnt].c1 = 'B';
									pmatch->arinf[pmatch->arcnt++].c2 = 'H';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
								if (fabs(bhandi1) - fabs(hhandi1) < 0) {
									//This means hga offer high value.
									if (hhandi1 > 0) {
										v1 = hhandi1;
										o1 = hfodd1;
										v2 = bhandi2;
										o2 = bfodd2;
										pmatch->inf[7][pmatch->mcnt[7]].c1 = 'H';
										pmatch->inf[7][pmatch->mcnt[7]].c2 = 'B';
									}
									else {
										v1 = hhandi2;
										o1 = hfodd2;
										v2 = bhandi1;
										o2 = bfodd1;
										pmatch->inf[7][pmatch->mcnt[7]].c1 = 'H';
										pmatch->inf[7][pmatch->mcnt[7]].c2 = 'B';
									}
								}
								else {
									if (bhandi1 > 0) {
										v1 = bhandi1;
										o1 = bfodd1;
										v2 = hhandi2;
										o2 = hfodd2;
										pmatch->inf[7][pmatch->mcnt[7]].c1 = 'B';
										pmatch->inf[7][pmatch->mcnt[7]].c2 = 'H';
									}
									else {
										v1 = bhandi2;
										o1 = bfodd2;
										v2 = hhandi1;
										o2 = hfodd1;
										pmatch->inf[7][pmatch->mcnt[7]].c1 = 'B';
										pmatch->inf[7][pmatch->mcnt[7]].c2 = 'H';
									}
								}
								stake1 = 100; stake2 = stake1 * o1 / o2;
								h1 = calc_middle(stake1, stake2, o1, o2, v1, v2);
								pmatch->inf[7][pmatch->mcnt[7]].middle = h1;
								pmatch->inf[7][pmatch->mcnt[7]].handi1 = v1;
								pmatch->inf[7][pmatch->mcnt[7]].handi2 = v2;;
								pmatch->inf[7][pmatch->mcnt[7]].odd1 = o1;
								pmatch->inf[7][pmatch->mcnt[7]++].odd2 = o2;
								pmatch->inf[7] = (middle_inf*)realloc(pmatch->inf[7], (pmatch->mcnt[7] + 1) * sizeof(middle_inf));
							}
						}
						////////////////////////////////////////////////////////////////////////////////
						// asian_total_cards
						///////////////////////////////////////////////////////////////////////////////					

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
							if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
								h1 = checkArbitrage(hfodd1, bfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(hhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = hfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = bfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 8;
									pmatch->arinf[pmatch->arcnt].c1 = 'H';
									pmatch->arinf[pmatch->arcnt++].c2 = 'B';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
								h1 = checkArbitrage(bfodd1, hfodd2);
								if (h1 <= MIN_ARBIT) {
									pmatch->arinf[pmatch->arcnt].ar = h1;
									pmatch->arinf[pmatch->arcnt].ov = fabs(bhandi1);
									pmatch->arinf[pmatch->arcnt].odd1 = bfodd1;
									pmatch->arinf[pmatch->arcnt].odd2 = hfodd2;
									pmatch->arinf[pmatch->arcnt].nwhere = 8;
									pmatch->arinf[pmatch->arcnt].c1 = 'B';
									pmatch->arinf[pmatch->arcnt++].c2 = 'H';
									pmatch->arinf = (arbit_inf*)realloc(pmatch->arinf, (pmatch->arcnt + 1) * sizeof(arbit_inf));
								}
							}
							if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
								if (bhandi1 < hhandi2) {
									v1 = bhandi1;
									o1 = bfodd1;
									v2 = hhandi2;
									o2 = hfodd2;
									pmatch->inf[8][pmatch->mcnt[8]].c1 = 'B';
									pmatch->inf[8][pmatch->mcnt[8]].c2 = 'H';
								}
								else {
									v1 = hhandi1;
									o1 = hfodd1;
									v2 = bhandi2;
									o2 = bfodd2;
									pmatch->inf[8][pmatch->mcnt[8]].c1 = 'H';
									pmatch->inf[8][pmatch->mcnt[8]].c2 = 'B';
								}
								stake1 = 100; stake2 = stake1 * o1 / o2;
								h1 = calc_middle_ou(stake1, stake2, o1, o2, v1, v2, 'o', 'u');
								pmatch->inf[8][pmatch->mcnt[8]].middle = h1;
								pmatch->inf[8][pmatch->mcnt[8]].handi1 = v1;
								pmatch->inf[8][pmatch->mcnt[8]].handi2 = v2;;
								pmatch->inf[8][pmatch->mcnt[8]].odd1 = o1;
								pmatch->inf[8][pmatch->mcnt[8]].nalt = 2;
								pmatch->inf[8][pmatch->mcnt[8]++].odd2 = o2;
								pmatch->inf[8] = (middle_inf*)realloc(pmatch->inf[8], (pmatch->mcnt[8] + 1) * sizeof(middle_inf));
							}
						}
					}
				}
			}
		}
		g_matches.push_back(pmatch);
		
		//break;
	}
	g_updated = 1;
	int nend_time = GetTickCount();
	memset(buff, 0, sizeof(wchar_t) * 200);
	swprintf(buff, 200, L"%d(%d) seconds. %d matches found.", (nend_time - nstart_time) / 1000, commtime / 1000, g_matches.size());
	SetDlgItemText(IDC_STATIC_COUNT, buff);
}
int COppCheckDlg::GetBet365_Odd(int match_id) {
	/*char buff[260] = { 0 };
	sprintf(buff, "ts%d.json", match_id);
	FILE *fp = fopen(buff, "rb");
	fseek(fp, 0, 2);
	int ns = ftell(fp);
	fseek(fp, 0, 0);
	char *pme = new char[ns + 1];
	memset(pme, 0, ns + 1);
	fread(pme, 1, ns, fp);
	fclose(fp);
	gs_strLastResponse = pme;
	return 0;*/
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
int COppCheckDlg::GetBet365_Odd1(int match_id) {
	char buff[260] = { 0 };
	sprintf(buff, "https://betsapi2.p.rapidapi.com/v3/bet365/prematch?FI=%d", match_id);
	CURL *hnd = curl_easy_init();

	curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(hnd, CURLOPT_URL, buff);

	struct curl_slist *headers = NULL;
	curl_easy_setopt(hnd, CURLOPT_CAINFO, "cacert.pem");
	headers = curl_slist_append(headers, "X-RapidAPI-Key: e2fa07ad89msh0857ce6a64b3678p13de20jsn4cdcdb04c8ff");
	headers = curl_slist_append(headers, "X-RapidAPI-Host: betsapi2.p.rapidapi.com");
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, function_pt1);        // set a callback to capture the server's response
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
		wcscpy(p, L"Asian Handicap Corners");
		break;
	case 5:
		wcscpy(p, L"Asian Total Corners");
		break;
	case 6:
		wcscpy(p, L"1st Half Asian Corners");
		break;
	case 7:
		wcscpy(p, L"Asian Handicap Cards");
		break;
	case 8:
		wcscpy(p, L"Asian Total Cards");
		break;
	default:
		break;
	}
}
int COppCheckDlg::check_filter(int idx) {
	if (goal_filter) {
		if (idx > 3)
			return 0;
	}
	if (corner_filter) {
		if (idx < 4 || idx > 6)
			return 0;
	}
	if (book_filter) {
		if (idx < 7)
			return 0;
	}
	return 1;
}
int COppCheckDlg::can_display(match_inf *pm) {
	int ncnt = pm->arcnt;
	int j, k;
	float middle;
	for (j = 0; j < CHECK_PARAMS; j++) {
		for (k = 0; k < pm->mcnt[j]; k++) {
			middle = pm->inf[j][k].middle;
			if (middle > g_nFilter) {
				ncnt++;
			}
		}
	}
	return ncnt;
}
DWORD COppCheckDlg::display_func() {
	int filelen1, ns;
	FILE *fhga;
	char *readBuffer1;
	wchar_t buff[260] = { 0 };
	int i, idx, j, narb, k, nmid;
	float middle;
	string s1, s2, s3;
	while (1)
	{
		while (1) {
			if (g_updated == 1)
				break;
			Sleep(1000);
		}
		ns = g_matches.size();
		if (ns == 0) {
			Sleep(500);
			continue;
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
		idx = 0;
		
		for (i = 0; i < ns; i++) {
			match_inf *pm = g_matches.at(i);
			if (can_display(pm) == 0)
				continue;
			string sdatetime = d[pm->hga_inx]["DATETIME"].GetString();
			string sleague = d[pm->hga_inx]["LEAGUE"].GetString();
			string steamh = d[pm->hga_inx]["TEAM_H"].GetString();
			string steamc = d[pm->hga_inx]["TEAM_C"].GetString();
			if (g_textFilter != "") {
				s1 = steamh;
				s2 = steamc;
				s3 = g_textFilter;
				for (auto & c : s1) c = toupper(c);
				for (auto & c : s2) c = toupper(c);
				for (auto & c : s3) c = toupper(c);
				if (!(s1.find(s3) != string::npos || s2.find(s3) != string::npos))
					continue;
			}
			TimeStamp2String(sdatetime.c_str(), buff);
			m_org.InsertItem(idx, buff);

			getWStr(sleague.c_str(), buff);
			m_org.SetItemText(idx, 1, buff);

			getWStr(steamh.c_str(), buff);
			m_org.SetItemText(idx, 2, buff);

			getWStr(steamc.c_str(), buff);
			m_org.SetItemText(idx, 3, buff);

			memset(buff, 0, 260);
			swprintf(buff, 260, L"%ld", (long int)(pm->pthis));
			m_org.SetItemText(idx,8, buff);

			memset(buff, 0, 260);
			swprintf(buff, 260, L"%d", pm->bid);
			m_org.SetItemText(idx, 9, buff);


			narb = 0; nmid = 0;
			for (j = 0; j < pm->arcnt; j++) {
				if (narb) {
					m_org.InsertItem(idx + narb, L"");
				}
				memset(buff, 0, 260);
				if(pm->arinf[j].nwhere == 0 || pm->arinf[j].nwhere == 2 || pm->arinf[j].nwhere == 4|| pm->arinf[j].nwhere == 7)
					swprintf(buff, 260, L"%.3f(%.2f)", pm->arinf[j].ar, pm->arinf[j].ov);
				else
					swprintf(buff, 260, L"%.3f(o/u%.2f)", pm->arinf[j].ar, pm->arinf[j].ov);
				m_org.SetItemText(idx + narb, 4, buff);

				memset(buff, 0, 260);
				getWhere(pm->arinf[j].nwhere, buff);
				m_org.SetItemText(idx + narb, 5, buff);

				memset(buff, 0, 260);
				swprintf(buff, 260, L"%.3f(%c)", pm->arinf[j].odd1, pm->arinf[j].c1);
				m_org.SetItemText(idx + narb, 6, buff);

				memset(buff, 0, 260);
				swprintf(buff, 260, L"%.3f(%c)", pm->arinf[j].odd2, pm->arinf[j].c2);
				m_org.SetItemText(idx + narb, 7, buff);
				narb++;
				
			}
			for (j = 0; j < CHECK_PARAMS; j++) {
				if (check_filter(j) == 0)
					continue;
				for (k = 0; k < pm->mcnt[j]; k++) {
					middle = pm->inf[j][k].middle;
					if (middle > g_nFilter) {
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

						if (pm->inf[j][k].nalt & 1)
							m_org.SetItemText(idx + narb + nmid, 2, L"AlternativeLine");						

						memset(buff, 0, 260);
						if(pm->inf[j][k].nalt & 2)
							swprintf(buff, 260, L"O%.2f", pm->inf[j][k].handi1);
						else
							swprintf(buff, 260, L"%.3f", pm->inf[j][k].handi1);
						m_org.SetItemText(idx + narb + nmid, 3, buff);

						memset(buff, 0, 260);
						if (pm->inf[j][k].nalt & 2)
							swprintf(buff, 260, L"U%.2f", pm->inf[j][k].handi2);
						else
							swprintf(buff, 260, L"%.3f", pm->inf[j][k].handi2);
						m_org.SetItemText(idx + narb + nmid, 4, buff);

						memset(buff, 0, 260);
						getWhere(j, buff);
						m_org.SetItemText(idx + narb + nmid, 5, buff);

						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f(%c)", pm->inf[j][k].odd1, pm->inf[j][k].c1);
						m_org.SetItemText(idx + narb + nmid, 6, buff);

						memset(buff, 0, 260);
						swprintf(buff, 260, L"%.3f(%c)", pm->inf[j][k].odd2, pm->inf[j][k].c2);
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
		
		m_status = 4;
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
	m_autochk.EnableWindow(TRUE);
	m_btn_refresh.EnableWindow(TRUE);
	return 0;
}
DWORD COppCheckDlg::catch_func_auto_func() {
	while (m_autorefresh) {
		m_autochk.EnableWindow(FALSE);
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
		m_autochk.EnableWindow(TRUE);
		Sleep(600 * 1000);
	}
	return 0;
}
DWORD COppCheckDlg::catch_thread(LPVOID param) {
	COppCheckDlg *pDlg = (COppCheckDlg*)param;
	if (pDlg == NULL)
		return 0;
	return pDlg->catch_func();
}
DWORD COppCheckDlg::catch_func_auto(LPVOID param) {
	COppCheckDlg *pDlg = (COppCheckDlg*)param;
	if (pDlg == NULL)
		return 0;
	return pDlg->catch_func_auto_func();
}
void COppCheckDlg::TimeStamp2String(const char *timestamp, wchar_t *strtime) {
	time_t tmer = atoi(timestamp);
	tm *tt = localtime(&tmer);
	memset(strtime, 0, 200 * sizeof(wchar_t));
	wsprintf(strtime, L"%04d-%02d-%02d %02d:%02d:%02d", tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday, tt->tm_hour, tt->tm_min, tt->tm_sec);
}
void COppCheckDlg::OnBnClickedButton1()
{

	CString txt;
	int n = m_seltype.GetCurSel();
	GetDlgItemText(IDC_EDIT_FILTER, txt);
	if (txt.GetLength() == 0 && n == 1) {
		MessageBox(L"Please input filter value");
		return;
	}
	
	if (n == 1) {
		g_nFilter = GetDlgItemInt(IDC_EDIT_FILTER);
	}
	else {
		char buff[260] = { 0 };
		WideCharToMultiByte(CP_ACP, 0, txt.GetBuffer(0), txt.GetLength(), buff, 260, 0, 0);
		g_textFilter = buff;
	}
	g_updated = 1;
}
void COppCheckDlg::load_single_match_hga_365() {
	int nsize,j, nalt, k, bid = 0;
	char *p;
	FILE *fp = fopen("hga/match.json", "rb");
	if (fp == NULL)
		return;
	fseek(fp, 0, SEEK_END);
	nsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	p = new char[nsize + 1];
	memset(p, 0, nsize + 1);
	fread(p, 1, nsize, fp);
	fclose(fp);

	Document d;
 	d.Parse(p);

	delete p;

	
	//bid = 127948843;
	if (pinterested_match != NULL) {
		bid = pinterested_match->bid;
	}
	else {
		if (new_int_match != NULL)
			bid = new_int_match->bid;
	}
	if (bid == 0)
		return;
	
	string sstrong, shandicap, shodd, scodd, sgoalo, sgoalu, sgoaloodd, sgoaluodd;
	int handi_alt = d["handicap_count"].GetInt();
	int half_alt = d["1sthalf_count"].GetInt();
	if (handi_alt == 0 || half_alt == 0)
		return;

	if (new_int_match != NULL) {
		delete new_int_match->arinf;
		for (j = 0; j < CHECK_PARAMS; j++)
			delete new_int_match->inf[j];
		delete new_int_match;
		new_int_match = NULL;
	}
	new_int_match = new match_inf;
	memset(new_int_match, 0, sizeof(match_inf));
	new_int_match->arcnt = 0;
	new_int_match->pthis = new_int_match;
	new_int_match->bid = bid;
	new_int_match->arinf = new arbit_inf;
	for (j = 0;j < CHECK_PARAMS; j++)
		new_int_match->inf[j] = new middle_inf;

	Document odds;
	float h1, stake1, stake2;
	gs_betresponse = "";
	int ndiff = GetBet365_Odd1(bid);
	if (ndiff == CURLE_OK) {
#ifdef TEST_MODE
		FILE *fp = fopen("betref.json", "wb");
		fwrite(gs_betresponse.c_str(), 1, gs_betresponse.length(), fp);
		fclose(fp);
#endif
		odds.Parse(gs_betresponse.c_str());
		if (odds.HasMember("results") && odds["results"][0].HasMember("asian_lines")) {
			const Value& sp = odds["results"][0]["asian_lines"]["sp"];

			////////////////////////////////////////////////////////////////////////////////////////////////
			// start check for asian handicap
			////////////////////////////////////////////////////////////////////////////////////////////////
			string bodd1, bodd2, bhandicap1, bhandicap2;
			float bhandi1, bhandi2, hhandi1, hhandi2, bfodd1, bfodd2, hfodd1, hfodd2;

			float h_handis1[10] = { 0 };
			float h_handis2[10] = { 0 };
			float h_odds1[10] = { 0 };
			float h_odds2[10] = { 0 };
			float b_handis1[20] = { 0 };
			float b_handis2[20] = { 0 };
			float b_odds1[20] = { 0 };
			float b_odds2[20] = { 0 };

			char alt_buf[20] = { 0 };
			int idx1, idx2;
			float v1, v2, o1, o2;

			if (sp.HasMember("asian_handicap")) {
				bodd1 = sp["asian_handicap"]["odds"][0]["odds"].GetString();
				bodd2 = sp["asian_handicap"]["odds"][1]["odds"].GetString();
				bhandicap1 = sp["asian_handicap"]["odds"][0]["handicap"].GetString();
				bhandicap2 = sp["asian_handicap"]["odds"][1]["handicap"].GetString();
				bhandi1 = get_handicap_from_string(bhandicap1.c_str());
				bhandi2 = get_handicap_from_string(bhandicap2.c_str());
				bfodd1 = atof(bodd1.c_str());
				bfodd2 = atof(bodd2.c_str());
				b_handis1[0] = bhandi1;
				b_handis2[0] = bhandi2;
				b_odds1[0] = bfodd1;
				b_odds2[0] = bfodd2;

				for (j = 0;j < handi_alt; j++) {
					sprintf(alt_buf, "handicap_%d", j + 1);
					const Value &altline = d[alt_buf];
					sstrong = altline["STRONG"].GetString();
					shandicap = altline["RATIO_R"].IsNull() ? "null" : altline["RATIO_R"].GetString();
					shodd = altline["ior_RH"].IsNull() ? "null" : altline["ior_RH"].GetString();
					scodd = altline["ior_RC"].IsNull() ? "null" : altline["ior_RC"].GetString();
					hfodd1 = atof(shodd.c_str());
					hfodd2 = atof(scodd.c_str());
					hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
					hhandi2 = -hhandi1;
					if (sstrong == "H") {
						hhandi1 = hhandi2;
						hhandi2 = -hhandi2;
					}
					h_handis1[j] = hhandi1;
					h_handis2[j] = hhandi2;
					h_odds1[j] = hfodd1;
					h_odds2[j] = hfodd2;
				}
				if (sp.HasMember("alternative_asian_handicap") && odds["results"][0].HasMember("others")) {
					const Value &others = odds["results"][0]["others"];
					ndiff = others.Size();
					for (j = 0; j < ndiff; j++) {
						if (others[j]["sp"].HasMember("alternative_asian_handicap")) {
							const Value& aoddes = others[j]["sp"]["alternative_asian_handicap"]["odds"];
							nalt = aoddes.Size();
							idx1 = 1; idx2 = 1;
							for (k = 0; k < nalt; k++) {
								string saodd = aoddes[k]["odds"].GetString();
								float aodd = atof(saodd.c_str());
								string aheader = aoddes[k]["header"].GetString();
								int ah = atoi(aheader.c_str());
								string ahandicap = aoddes[k]["handicap"].GetString();
								float ahandi = get_handicap_from_string(ahandicap.c_str());
								if (ah == 1) {
									b_handis1[idx1] = ahandi;
									b_odds1[idx1] = aodd;
									idx1++;
								}
								else if (ah == 2) {
									b_handis2[idx2] = ahandi;
									b_odds2[idx2] = aodd;
									idx2++;
								}
							}
							break;
						}
					}
				}
				Asian_Handicap(new_int_match, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2, 0);
			}
			///////////////////////////////////////////////////////////////////////////////////////////////
			// Check For goal lines
			///////////////////////////////////////////////////////////////////////////////////////////////
			if (sp.HasMember("goal_line")) {
				memset(h_handis1, 0, 4 * sizeof(float));
				memset(h_handis2, 0, 4 * sizeof(float));
				memset(h_odds1, 0, 4 * sizeof(float));
				memset(h_odds2, 0, 4 * sizeof(float));
				memset(b_handis1, 0, 20 * sizeof(float));
				memset(b_handis2, 0, 20 * sizeof(float));
				memset(b_odds1, 0, 20 * sizeof(float));
				memset(b_odds2, 0, 20 * sizeof(float));

				bodd1 = sp["goal_line"]["odds"][0]["odds"].GetString();
				bodd2 = sp["goal_line"]["odds"][1]["odds"].GetString();
				bhandicap1 = sp["goal_line"]["odds"][0]["name"].GetString();
				bhandicap2 = sp["goal_line"]["odds"][1]["name"].GetString();
				bhandi1 = get_handicap_from_string(bhandicap1.c_str());
				bhandi2 = get_handicap_from_string(bhandicap2.c_str());
				bfodd1 = atof(bodd1.c_str());
				bfodd2 = atof(bodd2.c_str());

				b_handis1[0] = bhandi1;
				b_handis2[0] = bhandi2;
				b_odds1[0] = bfodd1;
				b_odds2[0] = bfodd2;

				memset(alt_buf, 0, 20);
				for (j = 0;j < handi_alt; j++) {
					sprintf(alt_buf, "handicap_%d", j + 1);
					const Value &altline = d[alt_buf];

					sgoalo = altline["RATIO_OUO"].IsNull() ? "null" : altline["RATIO_OUO"].GetString();
					sgoalu = altline["RATIO_OUU"].IsNull() ? "null" : altline["RATIO_OUU"].GetString();
					sgoaloodd = altline["ior_OUH"].IsNull() ? "null" : altline["ior_OUH"].GetString();
					sgoaluodd = altline["ior_OUC"].IsNull() ? "null" : altline["ior_OUC"].GetString();

					hfodd1 = atof(sgoaloodd.c_str());
					hfodd2 = atof(sgoaluodd.c_str());
					sgoalo = ReplaceAll(sgoalo, "O", "");
					sgoalu = ReplaceAll(sgoalu, "U", "");
					hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
					hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');

					h_handis1[j] = hhandi1;
					h_handis2[j] = hhandi2;
					h_odds1[j] = hfodd1;
					h_odds2[j] = hfodd2;
				}
				if (sp.HasMember("alternative_goal_line") && odds["results"][0].HasMember("others")) {
					const Value &others = odds["results"][0]["others"];
					ndiff = others.Size();
					for (j = 0; j < ndiff; j++) {
						if (others[j]["sp"].HasMember("alternative_goal_line")) {
							const Value& aoddes = others[j]["sp"]["alternative_goal_line"]["odds"];
							nalt = aoddes.Size();
							idx1 = 1; idx2 = 1;
							for (k = 0; k < nalt; k++) {
								string saodd = aoddes[k]["odds"].GetString();
								float aodd = atof(saodd.c_str());
								string aheader = aoddes[k]["header"].GetString();
								string ahandicap = aoddes[k]["name"].GetString();
								float ahandi = get_handicap_from_string(ahandicap.c_str());
								if (aheader == "Over") {
									b_handis1[idx1] = ahandi;
									b_odds1[idx1] = aodd;
									idx1++;
								}
								else {
									b_handis2[idx2] = ahandi;
									b_odds2[idx2] = aodd;
									idx2++;
								}
							}
							break;
						}
					}
				}
				Asian_GoalLine(new_int_match, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2, 1);
			}
			//////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 1st asian handicap
			//////////////////////////////////////////////////////////////////////////////////////////////////////////
			if (sp.HasMember("1st_half_asian_handicap")) {
				memset(h_handis1, 0, 4 * sizeof(float));
				memset(h_handis2, 0, 4 * sizeof(float));
				memset(h_odds1, 0, 4 * sizeof(float));
				memset(h_odds2, 0, 4 * sizeof(float));
				memset(b_handis1, 0, 20 * sizeof(float));
				memset(b_handis2, 0, 20 * sizeof(float));
				memset(b_odds1, 0, 20 * sizeof(float));
				memset(b_odds2, 0, 20 * sizeof(float));
				bodd1 = sp["1st_half_asian_handicap"]["odds"][0]["odds"].GetString();
				bodd2 = sp["1st_half_asian_handicap"]["odds"][1]["odds"].GetString();
				bhandicap1 = sp["1st_half_asian_handicap"]["odds"][0]["handicap"].GetString();
				bhandicap2 = sp["1st_half_asian_handicap"]["odds"][1]["handicap"].GetString();
				bhandi1 = get_handicap_from_string(bhandicap1.c_str());
				bhandi2 = get_handicap_from_string(bhandicap2.c_str());
				bfodd1 = atof(bodd1.c_str());
				bfodd2 = atof(bodd2.c_str());

				b_handis1[0] = bhandi1;
				b_handis2[0] = bhandi2;
				b_odds1[0] = bfodd1;
				b_odds2[0] = bfodd2;
				memset(alt_buf, 0, 20);
				for (j = 0;j < half_alt; j++) {
					sprintf(alt_buf, "1sthalf_%d", j + 1);
					const Value &altline = d[alt_buf];
					sstrong = altline["STRONG"].GetString();
					shandicap = altline["RATIO_HR"].IsNull() ? "null" : altline["RATIO_HR"].GetString();
					shodd = altline["ior_HRH"].IsNull() ? "null" : altline["ior_HRH"].GetString();
					scodd = altline["ior_HRC"].IsNull() ? "null" : altline["ior_HRC"].GetString();
					hfodd1 = atof(shodd.c_str());
					hfodd2 = atof(scodd.c_str());
					hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
					hhandi2 = -hhandi1;
					if (sstrong == "H") {
						hhandi1 = hhandi2;
						hhandi2 = -hhandi2;
					}
					h_handis1[j] = hhandi1;
					h_handis2[j] = hhandi2;
					h_odds1[j] = hfodd1;
					h_odds2[j] = hfodd2;
				}
				//User alternative lines to calc middling.
				if (sp.HasMember("alternative_1st_half_asian_handicap") && odds["results"][0].HasMember("others")) {
					const Value &others = odds["results"][0]["others"];
					ndiff = others.Size();
					for (j = 0; j < ndiff; j++) {
						if (others[j]["sp"].HasMember("alternative_1st_half_asian_handicap")) {
							const Value& aoddes = others[j]["sp"]["alternative_1st_half_asian_handicap"]["odds"];
							nalt = aoddes.Size();
							idx1 = 1; idx2 = 1;
							for (k = 0; k < nalt; k++) {
								string saodd = aoddes[k]["odds"].GetString();
								float aodd = atof(saodd.c_str());
								string aheader = aoddes[k]["header"].GetString();
								int ah = atoi(aheader.c_str());
								string ahandicap = aoddes[k]["handicap"].GetString();
								float ahandi = get_handicap_from_string(ahandicap.c_str());
								if (ah == 1) {
									b_handis1[idx1] = ahandi;
									b_odds1[idx1] = aodd;
									idx1++;
								}
								else if (ah == 2) {
									b_handis2[idx2] = ahandi;
									b_odds2[idx2] = aodd;
									idx2++;
								}
							}
							break;
						}
					}
				}
				Asian_Handicap(new_int_match, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2, 2);
			}
			//////////////////////////////////////////////////////////////////////////////////
			//	1st asian goal line
			/////////////////////////////////////////////////////////////////////////////////
			if (sp.HasMember("1st_half_goal_line")) {
				memset(h_handis1, 0, 4 * sizeof(float));
				memset(h_handis2, 0, 4 * sizeof(float));
				memset(h_odds1, 0, 4 * sizeof(float));
				memset(h_odds2, 0, 4 * sizeof(float));
				memset(b_handis1, 0, 20 * sizeof(float));
				memset(b_handis2, 0, 20 * sizeof(float));
				memset(b_odds1, 0, 20 * sizeof(float));
				memset(b_odds2, 0, 20 * sizeof(float));

				bodd1 = sp["1st_half_goal_line"]["odds"][0]["odds"].GetString();
				bodd2 = sp["1st_half_goal_line"]["odds"][1]["odds"].GetString();
				bhandicap1 = sp["1st_half_goal_line"]["odds"][0]["name"].GetString();
				bhandicap2 = sp["1st_half_goal_line"]["odds"][1]["name"].GetString();
				bhandi1 = get_handicap_from_string(bhandicap1.c_str());
				bhandi2 = get_handicap_from_string(bhandicap2.c_str());

				bfodd1 = atof(bodd1.c_str());
				bfodd2 = atof(bodd2.c_str());

				b_handis1[0] = bhandi1;
				b_handis2[0] = bhandi2;
				b_odds1[0] = bfodd1;
				b_odds2[0] = bfodd2;

				memset(alt_buf, 0, 20);

				for (j = 0;j < half_alt; j++) {
					sprintf(alt_buf, "1sthalf_%d", j + 1);
					const Value &altline = d[alt_buf];

					sgoalo = altline["RATIO_HOUO"].IsNull() ? "null" : altline["RATIO_HOUO"].GetString();
					sgoalu = altline["RATIO_HOUU"].IsNull() ? "null" : altline["RATIO_HOUU"].GetString();
					sgoaloodd = altline["ior_HOUH"].IsNull() ? "null" : altline["ior_HOUH"].GetString();
					sgoaluodd = altline["ior_HOUC"].IsNull() ? "null" : altline["ior_HOUC"].GetString();

					hfodd1 = atof(sgoaloodd.c_str());
					hfodd2 = atof(sgoaluodd.c_str());
					sgoalo = ReplaceAll(sgoalo, "O", "");
					sgoalu = ReplaceAll(sgoalu, "U", "");
					hhandi1 = get_handicap_from_string(sgoalo.c_str(), '/');
					hhandi2 = get_handicap_from_string(sgoalu.c_str(), '/');
					
					h_handis1[j] = hhandi1;
					h_handis2[j] = hhandi2;
					h_odds1[j] = hfodd1;
					h_odds2[j] = hfodd2;
				}

				if (sp.HasMember("alternative_1st_half_goal_line") && odds["results"][0].HasMember("others")) {
					const Value &others = odds["results"][0]["others"];
					ndiff = others.Size();
					for (j = 0; j < ndiff; j++) {
						if (others[j]["sp"].HasMember("alternative_1st_half_goal_line")) {
							const Value& aoddes = others[j]["sp"]["alternative_1st_half_goal_line"]["odds"];
							nalt = aoddes.Size();
							idx1 = 1;idx2 = 2;
							for (k = 0; k < nalt; k++) {
								string saodd = aoddes[k]["odds"].GetString();
								float aodd = atof(saodd.c_str());
								string aheader = aoddes[k]["header"].GetString();
								string ahandicap = aoddes[k]["name"].GetString();
								float ahandi = get_handicap_from_string(ahandicap.c_str());
								if (aheader == "Over") {
									b_handis1[idx1] = ahandi;
									b_odds1[idx1] = aodd;
									idx1++;
								}
								else {
									b_handis2[idx2] = ahandi;
									b_odds2[idx2] = aodd;
									idx2++;
								}
							}
							break;
						}
					}
				}

				Asian_GoalLine(new_int_match, b_handis1, b_handis2, h_handis1, h_handis2, b_odds1, b_odds2, h_odds1, h_odds2, 3);
			}
			//////////////////////////////////////////////////////////////////////////////////
			// asian_handicap_corners
			//////////////////////////////////////////////////////////////////////////////////

			if (d.HasMember("CN_DATA")) {
				const Value& cn_data = d["CN_DATA"];
				sstrong = cn_data["STRONG"].IsNull() ? "null" : cn_data["STRONG"].GetString();
				shandicap = cn_data["RATIO_R"].IsNull() ? "null" : cn_data["RATIO_R"].GetString();
				if (shandicap != "null") {
					shodd = cn_data["ior_RH"].IsNull() ? "null" : cn_data["ior_RH"].GetString();
					scodd = cn_data["ior_RC"].IsNull() ? "null" : cn_data["ior_RC"].GetString();
					sgoalo = cn_data["RATIO_OUO"].IsNull() ? "null" : cn_data["RATIO_OUO"].GetString();
					sgoalu = cn_data["RATIO_OUU"].IsNull() ? "null" : cn_data["RATIO_OUU"].GetString();
					sgoaloodd = cn_data["ior_OUH"].IsNull() ? "null" : cn_data["ior_OUH"].GetString();
					sgoaluodd = cn_data["ior_OUC"].IsNull() ? "null" : cn_data["ior_OUC"].GetString();
					if (sp.HasMember("asian_handicap_corners")) {
						bodd1 = sp["asian_handicap_corners"]["odds"][0]["odds"].GetString();
						bodd2 = sp["asian_handicap_corners"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["asian_handicap_corners"]["odds"][0]["handicap"].GetString();
						bhandicap2 = sp["asian_handicap_corners"]["odds"][1]["handicap"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
						hhandi2 = -hhandi1;

						bfodd1 = atof(bodd1.c_str());
						bfodd2 = atof(bodd2.c_str());
						hfodd1 = atof(shodd.c_str());
						hfodd2 = atof(scodd.c_str());
						if (sstrong == "H") {
							hhandi1 = hhandi2;
							hhandi2 = -hhandi2;
						}
						if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
							h1 = checkArbitrage(hfodd1, bfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(hhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = hfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = bfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 4;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'H';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'B';
							
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(bhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = bfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = hfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 4;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'B';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'H';
								
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001)) {
							if (fabs(bhandi1) - fabs(hhandi1) < 0) {
								//This means hga offer high value.
								if (hhandi1 > 0) {
									v1 = hhandi1;
									o1 = hfodd1;
									v2 = bhandi2;
									o2 = bfodd2;
									new_int_match->inf[4][new_int_match->mcnt[4]].c1 = 'H';
									new_int_match->inf[4][new_int_match->mcnt[4]].c2 = 'B';
								}
								else {
									v1 = hhandi2;
									o1 = hfodd2;
									v2 = bhandi1;
									o2 = bfodd1;
									new_int_match->inf[4][new_int_match->mcnt[4]].c1 = 'H';
									new_int_match->inf[4][new_int_match->mcnt[4]].c2 = 'B';
								}
							}
							else {
								if (bhandi1 > 0) {
									v1 = bhandi1;
									o1 = bfodd1;
									v2 = hhandi2;
									o2 = hfodd2;
									new_int_match->inf[4][new_int_match->mcnt[4]].c1 = 'B';
									new_int_match->inf[4][new_int_match->mcnt[4]].c2 = 'H';
								}
								else {
									v1 = bhandi2;
									o1 = bfodd2;
									v2 = hhandi1;
									o2 = hfodd1;
									new_int_match->inf[4][new_int_match->mcnt[4]].c1 = 'B';
									new_int_match->inf[4][new_int_match->mcnt[4]].c2 = 'H';
								}
							}
							stake1 = 100; stake2 = stake1 * o1 / o2;
							h1 = calc_middle(stake1, stake2, o1, o2, v1, v2);
							new_int_match->inf[4][new_int_match->mcnt[4]].middle = h1;
							new_int_match->inf[4][new_int_match->mcnt[4]].handi1 = v1;
							new_int_match->inf[4][new_int_match->mcnt[4]].handi2 = v2;;
							new_int_match->inf[4][new_int_match->mcnt[4]].odd1 = o1;
							new_int_match->inf[4][new_int_match->mcnt[4]++].odd2 = o2;
							new_int_match->inf[4] = (middle_inf*)realloc(new_int_match->inf[4], (new_int_match->mcnt[4] + 1) * sizeof(middle_inf));
						}

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

						if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
							h1 = checkArbitrage(hfodd1, bfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(hhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = hfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = bfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 5;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'H';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'B';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(bhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = bfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = hfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 5;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'B';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'H';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
							if (bhandi1 < hhandi2) {
								v1 = bhandi1;
								o1 = bfodd1;
								v2 = hhandi2;
								o2 = hfodd2;
								new_int_match->inf[5][new_int_match->mcnt[5]].c1 = 'B';
								new_int_match->inf[5][new_int_match->mcnt[5]].c2 = 'H';
							}
							else {
								v1 = hhandi1;
								o1 = hfodd1;
								v2 = bhandi2;
								o2 = bfodd2;
								new_int_match->inf[5][new_int_match->mcnt[5]].c1 = 'H';
								new_int_match->inf[5][new_int_match->mcnt[5]].c2 = 'B';
							}
							stake1 = 100; stake2 = stake1 * o1 / o2;
							h1 = calc_middle_ou(stake1, stake2, o1, o2, v1, v2, 'o', 'u');

							new_int_match->inf[5][new_int_match->mcnt[5]].middle = h1;
							new_int_match->inf[5][new_int_match->mcnt[5]].handi1 = v1;
							new_int_match->inf[5][new_int_match->mcnt[5]].handi2 = v2;;
							new_int_match->inf[5][new_int_match->mcnt[5]].odd1 = o1;
							new_int_match->inf[5][new_int_match->mcnt[5]].nalt = 2;
							new_int_match->inf[5][new_int_match->mcnt[5]++].odd2 = o2;
							new_int_match->inf[5] = (middle_inf*)realloc(new_int_match->inf[5], (new_int_match->mcnt[5] + 1) * sizeof(middle_inf));
						}
					}
					////////////////////////////////////////////////////////////////
					// 1st_half_asian_corners
					///////////////////////////////////////////////////////////////
					if (sp.HasMember("asian_total_corners")) {
						sgoalo = cn_data["RATIO_HOUO"].IsNull() ? "null" : cn_data["RATIO_HOUO"].GetString();
						sgoalu = cn_data["RATIO_HOUU"].IsNull() ? "null" : cn_data["RATIO_HOUU"].GetString();
						sgoaloodd = cn_data["ior_HOUH"].IsNull() ? "null" : cn_data["ior_HOUH"].GetString();
						sgoaluodd = cn_data["ior_HOUC"].IsNull() ? "null" : cn_data["ior_HOUC"].GetString();

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
						if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
							h1 = checkArbitrage(hfodd1, bfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(hhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = hfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = bfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 6;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'H';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'B';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(bhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = bfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = hfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 6;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'B';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'H';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
							if (bhandi1 < hhandi2) {
								v1 = bhandi1;
								o1 = bfodd1;
								v2 = hhandi2;
								o2 = hfodd2;
								new_int_match->inf[6][new_int_match->mcnt[6]].c1 = 'B';
								new_int_match->inf[6][new_int_match->mcnt[6]].c2 = 'H';
							}
							else {
								v1 = hhandi1;
								o1 = hfodd1;
								v2 = bhandi2;
								o2 = bfodd2;
								new_int_match->inf[6][new_int_match->mcnt[6]].c1 = 'H';
								new_int_match->inf[6][new_int_match->mcnt[6]].c2 = 'B';
							}
							stake1 = 100; stake2 = stake1 * o1 / o2;
							h1 = calc_middle_ou(stake1, stake2, o1, o2, v1, v2, 'o', 'u');
							new_int_match->inf[6][new_int_match->mcnt[6]].middle = h1;
							new_int_match->inf[6][new_int_match->mcnt[6]].handi1 = v1;
							new_int_match->inf[6][new_int_match->mcnt[6]].handi2 = v2;;
							new_int_match->inf[6][new_int_match->mcnt[6]].odd1 = o1;
							new_int_match->inf[6][new_int_match->mcnt[6]].nalt = 2;
							new_int_match->inf[6][new_int_match->mcnt[6]++].odd2 = o2;
							new_int_match->inf[6] = (middle_inf*)realloc(new_int_match->inf[6], (new_int_match->mcnt[6] + 1) * sizeof(middle_inf));
						}
					}
				}
			}

			//////////////////////////////////////////////////////////////////////////////
			//  Asian handicap cards
			/////////////////////////////////////////////////////////////////////////////
			if (d.HasMember("RN_DATA")) {
				const Value& rn_data = d["RN_DATA"];
				sstrong = rn_data["STRONG"].IsNull() ? "null" : rn_data["STRONG"].GetString();
				shandicap = rn_data["RATIO_R"].IsNull() ? "null" : rn_data["RATIO_R"].GetString();
				if (shandicap != "null") {
					shodd = rn_data["ior_RH"].IsNull() ? "null" : rn_data["ior_RH"].GetString();
					scodd = rn_data["ior_RC"].IsNull() ? "null" : rn_data["ior_RC"].GetString();
					sgoalo = rn_data["RATIO_OUO"].IsNull() ? "null" : rn_data["RATIO_OUO"].GetString();
					sgoalu = rn_data["RATIO_OUU"].IsNull() ? "null" : rn_data["RATIO_OUU"].GetString();
					sgoaloodd = rn_data["ior_OUH"].IsNull() ? "null" : rn_data["ior_OUH"].GetString();
					sgoaluodd = rn_data["ior_OUC"].IsNull() ? "null" : rn_data["ior_OUC"].GetString();

					if (sp.HasMember("asian_handicap_cards")) {
						bodd1 = sp["asian_handicap_cards"]["odds"][0]["odds"].GetString();
						bodd2 = sp["asian_handicap_cards"]["odds"][1]["odds"].GetString();
						bhandicap1 = sp["asian_handicap_cards"]["odds"][0]["handicap"].GetString();
						bhandicap2 = sp["asian_handicap_cards"]["odds"][1]["handicap"].GetString();
						bhandi1 = get_handicap_from_string(bhandicap1.c_str());
						bhandi2 = get_handicap_from_string(bhandicap2.c_str());

						hhandi1 = get_handicap_from_string(shandicap.c_str(), '/');
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
						if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
							h1 = checkArbitrage(hfodd1, bfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(hhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = hfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = bfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 7;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'H';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'B';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(bhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = bfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = hfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 7;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'B';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'H';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
							if (fabs(bhandi1) - fabs(hhandi1) < 0) {
								//This means hga offer high value.
								if (hhandi1 > 0) {
									v1 = hhandi1;
									o1 = hfodd1;
									v2 = bhandi2;
									o2 = bfodd2;
									new_int_match->inf[7][new_int_match->mcnt[7]].c1 = 'H';
									new_int_match->inf[7][new_int_match->mcnt[7]].c2 = 'B';
								}
								else {
									v1 = hhandi2;
									o1 = hfodd2;
									v2 = bhandi1;
									o2 = bfodd1;
									new_int_match->inf[7][new_int_match->mcnt[7]].c1 = 'H';
									new_int_match->inf[7][new_int_match->mcnt[7]].c2 = 'B';
								}
							}
							else {
								if (bhandi1 > 0) {
									v1 = bhandi1;
									o1 = bfodd1;
									v2 = hhandi2;
									o2 = hfodd2;
									new_int_match->inf[7][new_int_match->mcnt[7]].c1 = 'B';
									new_int_match->inf[7][new_int_match->mcnt[7]].c2 = 'H';
								}
								else {
									v1 = bhandi2;
									o1 = bfodd2;
									v2 = hhandi1;
									o2 = hfodd1;
									new_int_match->inf[7][new_int_match->mcnt[7]].c1 = 'B';
									new_int_match->inf[7][new_int_match->mcnt[7]].c2 = 'H';
								}
							}
							stake1 = 100; stake2 = stake1 * o1 / o2;
							h1 = calc_middle(stake1, stake2, o1, o2, v1, v2);
							new_int_match->inf[7][new_int_match->mcnt[7]].middle = h1;
							new_int_match->inf[7][new_int_match->mcnt[7]].handi1 = v1;
							new_int_match->inf[7][new_int_match->mcnt[7]].handi2 = v2;;
							new_int_match->inf[7][new_int_match->mcnt[7]].odd1 = o1;
							new_int_match->inf[7][new_int_match->mcnt[7]++].odd2 = o2;
							new_int_match->inf[7] = (middle_inf*)realloc(new_int_match->inf[7], (new_int_match->mcnt[7] + 1) * sizeof(middle_inf));
						}
					}
					////////////////////////////////////////////////////////////////////////////////
					// asian_total_cards
					///////////////////////////////////////////////////////////////////////////////					

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
						if (fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001) {
							h1 = checkArbitrage(hfodd1, bfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(hhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = hfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = bfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 8;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'H';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'B';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001) {
							h1 = checkArbitrage(bfodd1, hfodd2);
							if (h1 <= MIN_ARBIT) {
								new_int_match->arinf[new_int_match->arcnt].ar = h1;
								new_int_match->arinf[new_int_match->arcnt].ov = fabs(bhandi1);
								new_int_match->arinf[new_int_match->arcnt].odd1 = bfodd1;
								new_int_match->arinf[new_int_match->arcnt].odd2 = hfodd2;
								new_int_match->arinf[new_int_match->arcnt].nwhere = 8;
								new_int_match->arinf[new_int_match->arcnt].c1 = 'B';
								new_int_match->arinf[new_int_match->arcnt++].c2 = 'H';
								new_int_match->arinf = (arbit_inf*)realloc(new_int_match->arinf, (new_int_match->arcnt + 1) * sizeof(arbit_inf));
							}
						}
						if (!(fabs(fabs(bhandi1) - fabs(hhandi2)) < 0.001 || fabs(fabs(hhandi1) - fabs(bhandi2)) < 0.001)) {
							if (bhandi1 < hhandi2) {
								v1 = bhandi1;
								o1 = bfodd1;
								v2 = hhandi2;
								o2 = hfodd2;
								new_int_match->inf[8][new_int_match->mcnt[8]].c1 = 'B';
								new_int_match->inf[8][new_int_match->mcnt[8]].c2 = 'H';
							}
							else {
								v1 = hhandi1;
								o1 = hfodd1;
								v2 = bhandi2;
								o2 = bfodd2;
								new_int_match->inf[8][new_int_match->mcnt[8]].c1 = 'H';
								new_int_match->inf[8][new_int_match->mcnt[8]].c2 = 'B';
							}
							stake1 = 100; stake2 = stake1 * o1 / o2;
							h1 = calc_middle_ou(stake1, stake2, o1, o2, v1, v2, 'o', 'u');
							new_int_match->inf[8][new_int_match->mcnt[8]].middle = h1;
							new_int_match->inf[8][new_int_match->mcnt[8]].handi1 = v1;
							new_int_match->inf[8][new_int_match->mcnt[8]].handi2 = v2;;
							new_int_match->inf[8][new_int_match->mcnt[8]].odd1 = o1;
							new_int_match->inf[8][new_int_match->mcnt[8]].nalt = 2;
							new_int_match->inf[8][new_int_match->mcnt[8]++].odd2 = o2;
							new_int_match->inf[8] = (middle_inf*)realloc(new_int_match->inf[8], (new_int_match->mcnt[8] + 1) * sizeof(middle_inf));
						}
					}
				}
			}
		}
	}
}
void COppCheckDlg::update_inter_table() {
	if (new_int_match == NULL)
		return;
	int filelen1;
	int idx = 0, j, narb = 0, k, nmid = 0;
	char *readBuffer1;
	wchar_t buff[260] = { 0 };
	float middle;
	FILE *fhga = fopen("hga/match.json", "rb"); // non-Windows use "r"
	if (fhga == NULL)
		return;
	fseek(fhga, 0, SEEK_END);
	filelen1 = ftell(fhga);
	fseek(fhga, 0, SEEK_SET);
	readBuffer1 = new char[filelen1 + 1];
	memset(readBuffer1, 0, filelen1 + 1);
	fread(readBuffer1, 1, filelen1, fhga);
	fclose(fhga);

	m_lstint.SetRedraw(FALSE);
	m_lstint.DeleteAllItems();
	m_lstint.SetRedraw(TRUE);

	Document d;
	d.Parse(readBuffer1);
	delete readBuffer1;
	
	int nhandi = d.HasMember("handicap_count") ? d["handicap_count"].GetInt() : 0;
	int nhalf = d.HasMember("1sthalf_count") ? d["1sthalf_count"].GetInt() : 0;
	if (nhandi == 0)
		return;
	string sdatetime = d["DATETIME"].GetString();
	string sleague = d["LEAGUE"].GetString();
	string steamh = d["TEAM_H"].GetString();
	string steamc = d["TEAM_c"].GetString();

	TimeStamp2String(sdatetime.c_str(), buff);
	m_lstint.InsertItem(idx, buff);

	getWStr(sleague.c_str(), buff);
	m_lstint.SetItemText(idx, 1, buff);

	getWStr(steamh.c_str(), buff);
	m_lstint.SetItemText(idx, 2, buff);

	getWStr(steamc.c_str(), buff);
	m_lstint.SetItemText(idx, 3, buff);
	
	for (j = 0; j < new_int_match->arcnt; j++) {
		if (narb) {
			m_lstint.InsertItem(idx + narb, L"");
		}
		memset(buff, 0, 260);
		if (new_int_match->arinf[j].nwhere == 0 || new_int_match->arinf[j].nwhere == 2 || new_int_match->arinf[j].nwhere == 4 || new_int_match->arinf[j].nwhere == 7)
			swprintf(buff, 260, L"%.3f(%.2f)", new_int_match->arinf[j].ar, new_int_match->arinf[j].ov);
		else
			swprintf(buff, 260, L"%.3f(o/u%.2f)", new_int_match->arinf[j].ar, new_int_match->arinf[j].ov);
		m_lstint.SetItemText(idx + narb, 4, buff);

		memset(buff, 0, 260);
		getWhere(new_int_match->arinf[j].nwhere, buff);
		m_lstint.SetItemText(idx + narb, 5, buff);

		memset(buff, 0, 260);
		swprintf(buff, 260, L"%.3f(%c)", new_int_match->arinf[j].odd1, new_int_match->arinf[j].c1);
		m_lstint.SetItemText(idx + narb, 6, buff);

		memset(buff, 0, 260);
		swprintf(buff, 260, L"%.3f(%c)", new_int_match->arinf[j].odd2, new_int_match->arinf[j].c2);
		m_lstint.SetItemText(idx + narb, 7, buff);


		narb++;
	}
	for (j = 0; j < CHECK_PARAMS; j++) {
		for (k = 0; k < new_int_match->mcnt[j]; k++) {
			middle = new_int_match->inf[j][k].middle;
			if (middle > g_nFilter) {
				if (nmid == 0) {
					if (narb == 0)
						narb = 1;

					m_lstint.InsertItem(idx + narb, L"");
					narb++;

					m_lstint.InsertItem(idx + narb, L"");
					m_lstint.SetItemText(idx + narb, 1, L"Middle");
					m_lstint.SetItemText(idx + narb, 2, L"");
					m_lstint.SetItemText(idx + narb, 3, L"Handicap1");
					m_lstint.SetItemText(idx + narb, 4, L"Handicap2");
					m_lstint.SetItemText(idx + narb, 5, L"Where");
					m_lstint.SetItemText(idx + narb, 6, L"Odd1");
					m_lstint.SetItemText(idx + narb, 7, L"Odd2");


					nmid++;
				}
				m_lstint.InsertItem(idx + narb + nmid, L"");


				memset(buff, 0, 260);
				swprintf(buff, 260, L"%.3f", middle);
				m_lstint.SetItemText(idx + narb + nmid, 1, buff);

				if (new_int_match->inf[j][k].nalt & 1)
					m_lstint.SetItemText(idx + narb + nmid, 2, L"AlternativeLine");

				memset(buff, 0, 260);
				if (new_int_match->inf[j][k].nalt & 2)
					swprintf(buff, 260, L"O%.2f", new_int_match->inf[j][k].handi1);
				else
					swprintf(buff, 260, L"%.3f", new_int_match->inf[j][k].handi1);
				m_lstint.SetItemText(idx + narb + nmid, 3, buff);

				memset(buff, 0, 260);
				if (new_int_match->inf[j][k].nalt & 2)
					swprintf(buff, 260, L"U%.2f", new_int_match->inf[j][k].handi2);
				else
					swprintf(buff, 260, L"%.3f", new_int_match->inf[j][k].handi2);
				m_lstint.SetItemText(idx + narb + nmid, 4, buff);

				memset(buff, 0, 260);
				getWhere(j, buff);
				m_lstint.SetItemText(idx + narb + nmid, 5, buff);

				memset(buff, 0, 260);
				swprintf(buff, 260, L"%.3f(%c)", new_int_match->inf[j][k].odd1, new_int_match->inf[j][k].c1);
				m_lstint.SetItemText(idx + narb + nmid, 6, buff);

				memset(buff, 0, 260);
				swprintf(buff, 260, L"%.3f(%c)", new_int_match->inf[j][k].odd2, new_int_match->inf[j][k].c2);
				m_lstint.SetItemText(idx + narb + nmid, 7, buff);
				nmid++;
			}
		}
	}
}
DWORD COppCheckDlg::inter_func() {
	char buff[1000] = { 0 };
	wchar_t cmdline[1000] = { 0 };
	SHELLEXECUTEINFO ShRun = { 0 };
//	WIN32_FIND_DATA fd;
	match_inf *pmatch;
	int nn1, nn2, nn3;
	while (1) {
		nn1 = GetTickCount();
		Sleep(1000);
		pmatch = (pinterested_match != NULL) ? pinterested_match : new_int_match;
		if (pmatch == NULL)
			continue;
		memset(buff, 0, 1000);
		memset(cmdline, 0, sizeof(wchar_t) * 1000);
		sprintf(buff, "match_infor_by_id.py %s %d %d", pmatch->showtype, pmatch->lid, pmatch->hid);
		MultiByteToWideChar(CP_ACP, 0, buff, strlen(buff), cmdline, 1000);
		
		
		memset(&ShRun, 0, sizeof(SHELLEXECUTEINFO));
		ShRun.cbSize = sizeof(SHELLEXECUTEINFO);
		ShRun.fMask = SEE_MASK_NOCLOSEPROCESS;
		ShRun.hwnd = NULL;
		ShRun.lpVerb = NULL;
		ShRun.lpFile = L"pythonw.exe";
		ShRun.lpParameters = cmdline;
		ShRun.lpDirectory = L"hga";
		ShRun.nShow = SW_SHOW;
		ShRun.hInstApp = NULL;
		// Execute the file with the parameters
		if (!ShellExecuteEx(&ShRun))
		{
		}
		Sleep(10000);

		load_single_match_hga_365();
		update_inter_table();
		nn2 = GetTickCount();
		nn3 = nn2 - nn1;
		nn3 = 0;
	}
}
DWORD COppCheckDlg::inter_thread(LPVOID param) {
	COppCheckDlg *pThis = (COppCheckDlg*)param;
	if (pThis == NULL)
		return 0;
	return pThis->inter_func();
}

void COppCheckDlg::OnBnClickedBtnAddto()
{
	// TODO: Add your control notification handler code here
	char buffs[1000] = { 0 };
	wchar_t cmdline[1000] = { 0 };
	SHELLEXECUTEINFO ShRun = { 0 };
	WIN32_FIND_DATA fd;
	match_inf *pmatch;
	int nn1, nn2, nn3;
	POSITION pos = m_org.GetFirstSelectedItemPosition();
	int selected = -1;
	if (pos != NULL)
	{
		while (pos)
		{
			int nItem = m_org.GetNextSelectedItem(pos);
			selected = nItem;
		}
	}
	if (!(m_status == 4 || m_status == 0)) {
		MessageBox(L"Can not add while downloading.");
		return;
	}
	wchar_t buff[1000] = { 0 };
	m_org.GetItemText(selected, 8, buff, 100);
	if (wcslen(buff) == 0) {
		MessageBox(L"Select correct line.");
		return;
	}
	int iid = _wtoi(buff);
	pinterested_match = (match_inf*)iid;
	MessageBox(L"Added to interested record. Please wait a few second...", L"Information", MB_ICONINFORMATION|MB_OK);
#ifdef TEST_MODE
	
	memset(buffs, 0, 1000);
	memset(cmdline, 0, sizeof(wchar_t) * 1000);
	sprintf(buffs, "match_infor_by_id.py %s %d %d", pinterested_match->showtype, pinterested_match->lid, pinterested_match->hid);
	MultiByteToWideChar(CP_ACP, 0, buffs, strlen(buffs), cmdline, 1000);


	memset(&ShRun, 0, sizeof(SHELLEXECUTEINFO));
	ShRun.cbSize = sizeof(SHELLEXECUTEINFO);
	ShRun.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShRun.hwnd = NULL;
	ShRun.lpVerb = NULL;
	ShRun.lpFile = L"pythonw.exe";
	ShRun.lpParameters = cmdline;
	ShRun.lpDirectory = L"hga";
	ShRun.nShow = SW_SHOW;
	ShRun.hInstApp = NULL;
	// Execute the file with the parameters
	if (!ShellExecuteEx(&ShRun))
	{
	}
	Sleep(10000);

	load_single_match_hga_365();
	update_inter_table();
	nn2 = GetTickCount();
#endif
}


void COppCheckDlg::OnBnClickedChkAuto()
{
	// TODO: Add your control notification handler code here
	int n = m_autochk.GetCheck();
	DWORD dwThreadId;
	if (n == 1) {
		m_autorefresh = 1;
		m_btn_refresh.EnableWindow(FALSE);
		m_hcatch_auto = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)catch_func_auto, this, 0, &dwThreadId);
	}
	else {
		m_autorefresh = 0;
		m_btn_refresh.EnableWindow(TRUE);
		if(m_hcatch_auto != INVALID_HANDLE_VALUE)
			TerminateThread(m_hcatch_auto, 0);
	}
}


void COppCheckDlg::OnBnClickedRefresh()
{
	// TODO: Add your control notification handler code here
	DWORD dwThreadId;
	m_autochk.SetCheck(0);
	m_autochk.EnableWindow(FALSE);
	m_btn_refresh.EnableWindow(FALSE);
	m_hcatch = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)catch_thread, this, 0, &dwThreadId);
}


void COppCheckDlg::OnBnClickedChkGoal()
{
	// TODO: Add your control notification handler code here
	goal_filter = !goal_filter;
	g_updated = 1;
}


void COppCheckDlg::OnBnClickedChkCorner()
{
	// TODO: Add your control notification handler code here
	corner_filter = !corner_filter;
	g_updated = 1;
}


void COppCheckDlg::OnBnClickedChkBook()
{
	// TODO: Add your control notification handler code here
	book_filter = !book_filter;
	g_updated = 1;
}
