
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "unzipdlg.h"
#include "wce\wcedef.h"
#include "user_string.h"			//ADD TODA

// 圧縮・解凍ダイアログ
BOOL onInit(HWND h, WPARAM wp, LPARAM lp);
BOOL onDestroy(HWND h, WPARAM wp, LPARAM lp);
BOOL onCancel(HWND h);

HWND hStatic1, hStatic2, hStatic3;
HWND hProgress;
HWND g_hDlg;

extern HINSTANCE g_hInst;
extern HWND g_hWnd;

BOOL CALLBACK dlgProc(HWND h, UINT m,
						 WPARAM wp, LPARAM lp)
{
	switch ( m ) {
	case WM_INITDIALOG:	return onInit(h, wp, lp);
	case WM_CLOSE:	
	case WM_DESTROY:	return onDestroy(h, wp, lp);

	case WM_COMMAND:
		switch ( wp ) {
		case IDCANCEL:	return onCancel(h);
		}
	}
	return FALSE;
}

BOOL onInit(HWND h, WPARAM wp, LPARAM lp)
{
	hStatic1 = GetDlgItem(h, IDC_STATIC1);
	hStatic2 = GetDlgItem(h, IDC_STATIC2);
	hStatic3 = GetDlgItem(h, IDC_STATIC3);
	hProgress = GetDlgItem(h, IDC_PROGRESS1);
	g_hDlg = h;

	return FALSE;
}

BOOL onDestroy(HWND h, WPARAM wp, LPARAM lp)
{
	return TRUE;
}

BOOL onCancel(HWND h)
{
	return TRUE;
}

void progressSetRange(int min, int max)
{
	if(hProgress==NULL) return;
	SendMessage(hProgress, PBM_SETRANGE,
		(WPARAM)0,
		MAKELPARAM(min, max));
	SendMessage(hProgress, PBM_SETSTEP,
		(WPARAM)1, 0);
}

void progressSetStep(int step)
{
	if(hProgress==NULL) return;
	SendMessage(hProgress, PBM_SETSTEP,
		(WPARAM)PBM_SETSTEP, 0L);
}

void progressSetPos(int pos)
{
	if(hProgress==NULL) return;
	SendMessage(hProgress, PBM_SETPOS,
		(WPARAM)pos, 0L);
}

void progressGetAStep(void)
{
	if(hProgress==NULL) return;
	SendMessage(hProgress,
		PBM_STEPIT, 0, 0L);
}

void dlgSetArchiveName(const char* name)
{
	static TCHAR temp[64];
#ifdef _WIN32_WCE
	TCHAR *t = wce_AToW(name);
	wsprintf(temp, L"Archive: %s", t);
	LocalFree(t);
#else
	wsprintf(temp, "Archive: %s", name);
#endif
	SetWindowText(hStatic1, temp);
	UpdateWindow( g_hDlg );
}

void dlgSetFileName(const char* name)
{
	static TCHAR temp[64];
#ifdef _WIN32_WCE
	TCHAR *t = wce_AToW(name);
	wsprintf(temp, L"File: %s", t);
	LocalFree(t);
#else
	wsprintf(temp, "File: %s", name);
#endif
	SetWindowText(hStatic2, temp);
	UpdateWindow( g_hDlg );
}

void dlgSetFileSize(const int totalSize, const int size)
{
	static int iTotalSize=0;
	static int iSize=0;
	static TCHAR temp[64];
	
	if(totalSize!=-1)
		iTotalSize = totalSize;

	if(size!=-1)
		iSize = size;
		
	wsprintf(temp, _T("Size: %d:%d"), iSize, iTotalSize);
	
	SetWindowText(hStatic3, temp);
	UpdateWindow( g_hDlg );
}

/* 上書きしますか？ダイアログ */
//int createInquireDialog(const char* a, time_t t, WORD dat, WORD tim)
int createInquireDialog(const char* a, long t, WORD dat, WORD tim)
{
	int rc;
	static TCHAR lpTemp[3][128];
	SYSTEMTIME s;
	struct tm *tm;

#ifdef _WIN32_WCE
	wce_AToW2(a, lpTemp[0]);
	tm = localtime((const unsigned long*)&t);
#else
	strcpy(lpTemp[0], a);
	tm = localtime((const long*)&t);
#endif
	DosDateTimeToSystemTime( dat, tim, &s );
//	tm = localtime((const unsigned long*)&t);

	wsprintf(lpTemp[1],
		IDS_MSG_OVERWRITE_FROM,	//MOD TODA
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	wsprintf(lpTemp[2],
		IDS_MSG_OVERWRITE_TO,	//MOD TODA
		s.wYear, s.wMonth, s.wDay,
		s.wHour, s.wMinute, s.wSecond);
	
	rc = DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_INQUIRE_DIALOG),
		g_hWnd, (DLGPROC)inqDlgProc, (LPARAM)lpTemp);
	return rc;
}

BOOL CALLBACK inqDlgProc(HWND h, UINT m,
						 WPARAM wp, LPARAM lp)
{
	TCHAR* t[3];

	switch ( m ) {
	case WM_INITDIALOG:
		t[0] = (TCHAR*)lp;
		//MOD START TODA
//		t[1] = t[0] + 128*sizeof(TCHAR);
//		t[2] = t[1] + 128*sizeof(TCHAR);
		t[1] = ((TCHAR*)lp)+128;
		t[2] = ((TCHAR*)lp)+128*2;
		//MOD END
		SetWindowText(GetDlgItem(h, IDC_STATIC3), t[0]);
		SetWindowText(GetDlgItem(h, IDC_STATIC1), t[1]);
		SetWindowText(GetDlgItem(h, IDC_STATIC2), t[2]);
		return FALSE;

	// yes : 上書きする
	//  no : 上書きしないで次のファイルへ
	// all : すべて yes
	//skip : 上書きしないで終了
	case WM_COMMAND:
		switch ( wp ) {
		case IDYES:  EndDialog(h, 0); return TRUE;
		case IDNO:   EndDialog(h, 2); return TRUE;
		case IDALL:  EndDialog(h, 4); return TRUE;
		case IDSKIP: EndDialog(h, 6); return TRUE;
		}
		return FALSE;
	case WM_KEYDOWN:
		return TRUE;
	}
	return FALSE;
}
