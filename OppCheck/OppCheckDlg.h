
// OppCheckDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#define CHECK_PARAMS	9
#define MAX_MIDDLES	100

typedef struct middle_inf {
	float middle;
	float odd1;
	float odd2;
	float handi1;
	float handi2;
	int nalt;	//if 1 then this is alternative line.
	char c1;
	char c2;
	middle_inf() {
		odd1 = 0;
		odd2 = 0;
		handi1 = 0;
		handi2 = 0;
		middle = 0;
		nalt = 0;
	}
};
typedef struct arbit_inf {
	float ar;
	float odd1;
	float odd2;
	char c1;
	char c2;
	int nwhere;
	float ov;
	arbit_inf() {
		ar = 0;
		odd1 = 0;
		ov = 0;
		odd2 = 0;
		c1 = 0;
		c2 = 0;
	}
};
typedef struct match_inf {
	int hga_inx;
	int bid;
	int hid;
	int arcnt;
	int lid;
	match_inf *pthis;
	char showtype[6];
	arbit_inf *arinf;
	int mcnt[CHECK_PARAMS];
	middle_inf *inf[CHECK_PARAMS];
	match_inf() {
		arcnt = 0;
		hga_inx = 0;
		bid = 0;
		hid = 0;
		memset(mcnt, 0, sizeof(int) * CHECK_PARAMS);
	}
} MATCH_INF;
// COppCheckDlg dialog
class COppCheckDlg : public CDialogEx
{
// Construction
public:
	COppCheckDlg(CWnd* pParent = NULL);	// standard constructor
	~COppCheckDlg();

	void GetHGA();
	void GetBet365_Upcoming();
	int GetBet365_Odd(int match_id);
	int GetBet365_Odd1(int match_id);

	void ReadDataDisplay();
	void CalculateResult();
	void Initlist(); 
	void TimeStamp2String(const char *timestamp, wchar_t *strtime);
	void getWStr(const char *timestamp, wchar_t *wstr);
	void clear_match();
	float get_handicap_from_string(const char *p, int deli);
	int checkWin(float myhandicap, int mygoal, int op_goal);
	float get_profit(float odd, float myhandicap, float money, int mygoal, int op_goal);
	int checkOU(char t, float v, int goal);
	float calc_middle(float stake1, float stake2, float odd1, float odd2, float handicap1, float handicap2);
	float calc_middle_ou(float stake1, float stake2, float odd1, float odd2, float handicap1, float handicap2, char c1, char c2);
	float checkArbitrage(float odd1, float odd2);
	float checkArbitrage3(float a1, float a2, float a3);
	void getWhere(int n, wchar_t *p);
	void OnOK();
	void Asian_Handicap(match_inf *pmatch, float *bhandi1, float *bhandi2, float *hhandi1, float *hhandi2, 
		float *bfodd1, float *bfodd2, float *hfodd1, float *hfodd2, int inx);
	void Asian_GoalLine(match_inf *pmatch, float *bhandi1, float *bhandi2, float *hhandi1, float *hhandi2,
		float *bfodd1, float *bfodd2, float *hfodd1, float *hfodd2, int inx);
	void load_single_match_hga_365();
	void update_inter_table();
	int can_display(match_inf *pm);
	int check_filter(int idx);

	int m_status;
	HANDLE m_hcatch;
	HANDLE m_hcatch_auto;
	DWORD catch_func();
	DWORD catch_func_auto_func();
	static DWORD catch_thread(LPVOID param);
	static DWORD catch_func_auto(LPVOID param);
	HANDLE m_hdisplay;

	DWORD display_func();
	static DWORD display_thread(LPVOID param);
	DWORD inter_func();
	static DWORD inter_thread(LPVOID param);
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OPPCHECK_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	CListCtrl m_org;
	CProgressCtrl m_show;
	CEdit m_filteredit;
	afx_msg void OnBnClickedBtnAddto();
	CListCtrl m_lstint;
	afx_msg void OnBnClickedChkAuto();
	afx_msg void OnBnClickedRefresh();
	CButton m_autochk;
	CButton m_btn_refresh;
	CComboBox m_seltype;
	afx_msg void OnBnClickedChkGoal();
	afx_msg void OnBnClickedChkCorner();
	afx_msg void OnBnClickedChkBook();
};
