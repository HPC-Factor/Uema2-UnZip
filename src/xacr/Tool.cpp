// tool.cpp
// 　復元ルーチンの共通インターフェイスクラス。

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

//#include "stdafx.h"
//#include "resource.h"
#include "Tool.h"
#ifdef _WIN32_WCE
  #include "..\wce\wcedef.h"
#else
  #include <time.h>
#endif

extern HINSTANCE g_hInst;
 
#include "..\resource.h"

unsigned char* CTool::common_buf;

char* CTool::GetPassword()
{
	if(m_pPass!=NULL){
		delete [] m_pPass;
		m_pPass = NULL;
	}
//	DialogBoxParam( GetModuleHandle(NULL), g_hInst
	DialogBoxParam( g_hInst,
					MAKEINTRESOURCE(IDD_PASSWORD_DIALOG),
					NULL,PassProc,
					(LPARAM)(&m_pPass) );
	return m_pPass;
}

BOOL CALLBACK CTool::PassProc( HWND dlg,UINT msg,WPARAM wp,LPARAM lp )
{
	if( msg==WM_INITDIALOG )
	{
		SetWindowLong( dlg, GWL_USERDATA, lp );
		SendDlgItemMessage( dlg, IDC_MASK, BM_SETCHECK, BST_CHECKED, 0 );
		SendDlgItemMessage( dlg,IDC_PASSWORD,
							EM_SETPASSWORDCHAR, _T('*'),0 );
		SetFocus( GetDlgItem(dlg,IDC_PASSWORD) );
//		kiutil::wndFront( dlg );
		return TRUE;
	}
	else if( msg==WM_COMMAND )
	{
		if( LOWORD(wp)==IDOK )
		{
			TCHAR tPass[81];
			int len = SendDlgItemMessage( dlg,IDC_PASSWORD,
										WM_GETTEXTLENGTH,0,0);

			char** ppPass=(char**)GetWindowLong(dlg,GWL_USERDATA);
			*ppPass=new char[1+len];

			SendDlgItemMessage(dlg,IDC_PASSWORD,WM_GETTEXT,
//									len+1,(LPARAM)*ppPass);
									len+1,(LPARAM)tPass);
#ifdef _WIN32_WCE
			wce_AToW2(*ppPass, tPass);
#else //_WIN32_WCE
			strcpy(*ppPass, tPass);
#endif //_WIN32_WCE

			EndDialog(dlg,IDOK);
			return TRUE;
		}
		else if( LOWORD(wp)==IDC_MASK )
		{
			if( BST_CHECKED==
					SendDlgItemMessage( dlg,IDC_MASK,
										BM_GETCHECK,0,0 ) )
				SendDlgItemMessage( dlg,IDC_PASSWORD,
									EM_SETPASSWORDCHAR,_T('*'),0 );
			else
				SendDlgItemMessage( dlg,IDC_PASSWORD,
									EM_SETPASSWORDCHAR,0,0 );
			InvalidateRect( GetDlgItem(dlg,IDC_PASSWORD),
												NULL,TRUE );
			return TRUE;
		}
	}
	return FALSE;
}

void timeSet( const char* fname, FILETIME* pft )
{
#ifdef _WIN32_WCE
	TCHAR *t_fname = wce_AToW(fname);
	HANDLE han = CreateFile( t_fname, //fname,
#else
	HANDLE han = CreateFile( fname,
#endif
							 GENERIC_READ | GENERIC_WRITE,
							 FILE_SHARE_READ,NULL,
							 OPEN_EXISTING,
							 FILE_ATTRIBUTE_NORMAL,
							 NULL );
#ifdef _WIN32_WCE
	LocalFree(t_fname);
#endif
	if( han==INVALID_HANDLE_VALUE )
		return;

	SetFileTime( han,pft,NULL,pft );

	CloseHandle( han );
}

void timeSet( const char* fname, DWORD sec )
{
	//localtimeを使う。で、gmtimeと合わせるため、
	//時間を9時間減らす。by uema2.
	sec -= 60*60*9;
//	struct tm* time=gmtime((long*)&sec);
#ifdef _WIN32_WCE
	struct tm* time=localtime(&sec);
#else
	struct tm* time=localtime((const long*)&sec);
#endif

//	struct tm* time=gmtime((long*)&sec);
	if( time!=NULL )
	{
		FILETIME ft;
		SYSTEMTIME sys;

		sys.wYear =      time->tm_year+1900;
		sys.wMonth =     time->tm_mon+1;
		sys.wDayOfWeek = time->tm_wday;
		sys.wDay =       time->tm_mday;
		sys.wHour =      time->tm_hour;
		sys.wMinute =    time->tm_min;
		sys.wSecond =    time->tm_sec;
		sys.wMilliseconds = 0;
		SystemTimeToFileTime(&sys,&ft);
		timeSet( fname,&ft );
	}
}

void timeSet( const char* fname,WORD date,WORD time )
{
#ifdef _WIN32_WCE
	SYSTEMTIME s={0};
	FILETIME f={0};
	if( DosDateTimeToSystemTime( date, time, &s ) )
	{
		if( SystemTimeToFileTime( &s, &f ) )
		{
			if( LocalFileTimeToFileTime( &f, &f ) )	//ADD TODA
				timeSet( fname,&f );
		}
	}
#else
	FILETIME ft,lc;
	if( DosDateTimeToFileTime( date, time, &lc ) )
	{
		if( LocalFileTimeToFileTime( &lc, &ft ) )
			timeSet( fname,&ft );
	}
#endif
}

char lb[256];

void pathInit()
{
	lb[0] = 0;
	for( int c=1; c!=256; c++ )
		lb[c] = (IsDBCSLeadByte(c) ? 2 : 1);
}

#define isdblb(c) (lb[(unsigned char)(c)]==2)
#define step(p) (p+=lb[(unsigned char)*(p)])

char* pathMake( char* path )
{
	char* st = path;

	while( *st=='/' || *st=='\\' || *st=='?' )
		st++;
	// win フルパス対応用。CE には必要ないと思われる
//	if( st[0]!='\0' && st[1]==':' )
//		st+=2;
	while( *st=='/' || *st=='\\' || *st=='?' )
		st++;

	for( unsigned char *p=(unsigned char*)st; *p!='\0'; step(p) )
	{
		if( isdblb(*p) )
			continue;

		if( *p=='\\' || *p=='/' )
		{
			*p='\0';
#ifdef _WIN32_WCE
			TCHAR *t = wce_AToW( st );
			CreateDirectory( t, NULL );
			LocalFree( t );
#else
			CreateDirectory( st, NULL );
#endif
			*p='\\';
		}
		// win フルパス用対応。CE には必要ないものと思われる
		else if( *p<' ' || ( *p>'~' && !( 0xa0<=*p && *p<=0xdf ) ) || strchr("*?\"<>|",*p) ) //|| strchr(":*?\"<>|",*p) )
			*p = '_';
	}

	return st;
}

void pathMakeAbs( char* path )
{
	int i=0;
	for( char* p=path; *p!='\0'; step(p) )
	{
		if( i++ < 4 ) // 最初の４文字以内の \ はドライブを表す、ということにしておく。
			continue;

		if( *p=='\\' )
		{
			*p='\0';
#ifdef _WIN32_WCE
			TCHAR *t = wce_AToW( path );
			CreateDirectory( t, NULL );
			LocalFree( t );
#else
			CreateDirectory( path, NULL );
#endif
			*p='\\';
		}
	}
}
