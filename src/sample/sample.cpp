
#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#define puts(S) MessageBox(NULL, S, NULL, MB_OK);

typedef int (*UNZIP)(HWND, LPCTSTR, LPTSTR ,DWORD); 
typedef WORD (*FUNC)(VOID); 

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					 LPTSTR lpszCmdLine, int nCmdShow)
{

    HMODULE hDLL = ::LoadLibrary( _T("unzip.dll") ) ;

	if ( hDLL == NULL )
	{
		puts( _T("unzip.dll が見つかりません。") ) ;
		return 1 ;
	}

//	puts( _T("unzip.dll load ok.") ) ;

	FUNC func = (FUNC)::GetProcAddress( hDLL, _T("UnZipGetVersion") );
	if ( func )
	{
		DWORD dwVersion = (*func)();
    } else {

		DWORD d = GetLastError();
		TCHAR temp[64];
		wsprintf(temp, _T("GetProcAddress Error : %d"), d);
		puts(temp);
		return 1;
	}

	UNZIP unzip = (UNZIP)::GetProcAddress( hDLL, _T("UnZip") );
    if ( unzip )
    {
		static TCHAR outbuf[64*1024];
//		int ret = (*unzip)(NULL, _T("t test.zip test\\test.txt test\\test2.txt"), // チェック
//		int ret = (*unzip)(NULL, _T("lq test.zip"), // リスト表示
		int ret = (*unzip)(NULL, _T("x \\test.zip \\temp"), // 解凍
			outbuf, sizeof(outbuf));
		OutputDebugString(outbuf);
    }

	::FreeLibrary( hDLL ) ;

	return 0;
}

