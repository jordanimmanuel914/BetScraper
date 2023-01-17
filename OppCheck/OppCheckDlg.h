
// OppCheckDlg.h : header file
//

#pragma once
#include "afxcmn.h"


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

	int m_status;
	HANDLE m_hcatch;
	DWORD catch_func();
	static DWORD catch_thread(LPVOID param);
	HANDLE m_hdisplay;
	DWORD display_func();
	static DWORD display_thread(LPVOID param);
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
};
