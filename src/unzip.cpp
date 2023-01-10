
#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
//#include <crtdbg.h> 
#include "user_string.h"			//ADD TODA


LPTSTR g_outBuff;
DWORD  g_outBuffSize;
BOOL g_bRunning;
HINSTANCE g_hInst;
HWND g_hWnd;

// Win 上で free すると
// なぜか落ちるんでリークさせっぱなし(極悪)
//#define AVIOD_MEMFREE

#include "wce\wcedef.h"
#include "unzip.h"
#include "unzipdlg.h"
#include "resource.h"
#include "decMain.h"
#include "xacr\ZipTool.h"
#include "unzipdlg.h"


int getWildCards(const TCHAR* a, char **argv, int startNum);
extern int encodeMain(int, char**);
extern int decodeMain(int, char**);

//-------------------------------------------------------------------
// Unlha
int _export 
UnZip(
	const HWND	hwnd, 		// 親ウィンドウのハンドル
	LPCTSTR		szCmdLine,	// コマンドライン
	LPTSTR		szOutput, 	// 結果を返すためのバッファ
	const DWORD	dwSize		// バッファのサイズ
)
{
/*
	圧縮
	  a [archive-name] [filename]
	展開
	  e [archive-name] [dir] [filename]
*/
	HWND hDlg;
	static TCHAR temp[MAX_PATH*2];
	TCHAR *cmdLine;
	char *argvBuff[256];
	char **argv = NULL;
	TCHAR *pCmdLine, *pCmdStart;
	char argc=0;
	int i=0, count = 0, num=0;	//count:引数のスペースの数
								// num :総引数
	int returnCode = 0;
	INITCOMMONCONTROLSEX cc={sizeof(INITCOMMONCONTROLSEX)};
	HINSTANCE h;
	DWORD dwError;
	BOOL bDoubleQuote = FALSE;


	cc.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&cc);

	g_outBuff = szOutput;
	g_outBuffSize = dwSize;

	if( _tcslen(szCmdLine)==0){
		printfToBuffer(IDS_TITLE_DLL);			//MOD TODA
		return 1;
	}

	h = LoadLibrary( _T("unzip.dll") ) ;

	if( h == NULL )	return -1;

	g_hWnd = hwnd;
	g_hInst = h;
	g_bRunning = TRUE;
	pathInit();
	CTool::AllocWorkSpace();

	cmdLine = (TCHAR *)LocalAlloc(LPTR,
		(_tcslen(szCmdLine)+3)*sizeof(TCHAR));

	_tcscpy(cmdLine, szCmdLine);

	g_outBuff = szOutput;
	g_outBuffSize = dwSize;

	pCmdLine = cmdLine;
	while(1){
		if(*pCmdLine==_T('\"'))
			bDoubleQuote = bDoubleQuote==FALSE ? TRUE : FALSE;

		// 空白か NULL が、単語の区切りである
		if( (*pCmdLine==_T(' ') || *pCmdLine==_T('\0') ) &&
			bDoubleQuote==FALSE ){
			count++;
/*			if( count == 3 && 
				(*(pCmdLine-1) == _T('\\') ) || 
				(*(pCmdLine-1) == _T('\"') && *(pCmdLine-2) == _T('\\')) ){
				g_bCurrentDirectory = TRUE;
			}
*/		}
		if( *pCmdLine==_T('\0') ) break;
		pCmdLine = CharNext(pCmdLine);
	}

	argc = count+1;
	bDoubleQuote = FALSE;

	if( cmdLine[0] != _T('-') ){
		wsprintf(cmdLine, _T("-%s"), szCmdLine);
	}

	argvBuff[num] = (char*)LocalAlloc(LPTR, strlen("unzip.dll")+1);
	strcpy(argvBuff[num++], "unzip.dll");

	pCmdLine = cmdLine;
	int charlength=0;
	for(i=1; i<argc; i++){
//		n=0;
		pCmdStart = pCmdLine;
		while(1){
			if( (*pCmdLine==_T(' ') && !bDoubleQuote) ||
				*pCmdLine==_T('\0') ){
				if( *pCmdStart == _T('\"') ){
					pCmdStart = CharNext(pCmdStart);
//					n -= 2;
				}
				charlength = pCmdLine - pCmdStart;
				if( *(pCmdLine-1)==_T('\"') ) charlength--;
				_tcsncpy(temp, pCmdStart, charlength); // n);
				temp[charlength]=_T('\0');
				if( _tcschr(temp, _T('*')) ){
					// ワイルドカードありのときはそれを展開
					num = getWildCards(temp, argvBuff, num);
				}else{
					// 無いときはそのまま
#ifdef _WIN32_WCE
					// CE 用処理
					argvBuff[num++] = wce_WToA(temp);
#else
					// 念のため PC 用処理も書いておく
					argvBuff[num] = (char*)LocalAlloc(LPTR, charlength+1);
					strncpy(argvBuff[num], pCmdStart, charlength); // n);
					argvBuff[num++][charlength]=_T('\0');
#endif
				}
				pCmdLine = CharNext(pCmdLine);
				break;
			}else{
				if( *pCmdLine==_T('\"') )
					bDoubleQuote = bDoubleQuote==TRUE ? FALSE : TRUE;
//				n++;
				pCmdLine = CharNext(pCmdLine);
			}
		} //while
	}

	argv = argvBuff;
	argc = num;

	if( strchr(argv[1], 'q')==NULL ){
		// q オプションが指定されていなければ
		// ダイアログを出す
		hDlg = CreateDialog(h, 
			MAKEINTRESOURCE(IDD_PROGRESS_DIALOG),
			hwnd,
			(DLGPROC)dlgProc);

		if(hDlg==NULL){
			dwError = GetLastError();
			printfToBuffer(IDS_FMT_DLG_ERROR1);			//MOD TODA
			return -1;
		}
		ShowWindow(hDlg,SW_SHOW);

		//ダイアログのタイトルを決める
		if(0==strcmp(argv[1], "-a"))
			SetWindowText(hDlg, IDS_TITLE_COMPRESS);	//MOD TODA
		else
			SetWindowText(hDlg, IDS_TITLE_UNCOMPRESS);	//MOD TODA

	}else{
		hDlg = NULL;
	}

	// 圧縮・解凍ルーチンに処理を渡す
	if(strchr(argv[1], 'a')){
		// 圧縮(minizip)
		returnCode = encodeMain(argc, argv);
	}else{
		// 解凍(XacRett)
		returnCode = decodeMain(argc, argv);
	}

	if(CTool::common_buf!=NULL)
		CTool::FreeWorkSpace();

	// 終わったらダイアログを消す
	if( hDlg!=NULL ) DestroyWindow(hDlg);

	//

#ifndef AVIOD_MEMFREE
	// メモリ解放
	for(i=0; i<argc; i++){
		LocalFree( argv[i] );
	}
#endif
	LocalFree (cmdLine);

	// DLL をアンロード
    FreeLibrary( h );

	g_bRunning = FALSE;

#ifdef _DEBUG
//	_CrtDumpMemoryLeaks();
#endif

	/* 正常終了したら 0 */
	return returnCode;
}

//-------------------------------------------------------------------
// UnZipGetVersion
WORD _export 
UnZipGetVersion()
{
	return UNZIPCE_DLL_VERSION;
}

//-------------------------------------------------------------------
// UnZipGetRunning
BOOL _export UnZipGetRunning()
{
	return g_bRunning;
}

//-------------------------------------------------------------------
// UnZipCheckArchive
BOOL _export UnZipCheckArchive(LPCTSTR szFileName,const int iMode)
{
	CZipTool z;

	if(CTool::common_buf==NULL){
		pathInit();
		CTool::AllocWorkSpace();
	}

	// file open & read
	HANDLE h = CreateFile(szFileName, GENERIC_READ, 
		0, NULL, OPEN_EXISTING, 0, NULL);

	if( h == INVALID_HANDLE_VALUE ){
		printfToBuffer(IDS_FMT_DLG_ERROR2);			//MOD TODA
		return FALSE;
	}

	DWORD dwRead;
	ReadFile( h, CTool::common_buf, XACR_BUFSIZE, &dwRead, NULL);
	DWORD dwSize = GetFileSize( h, NULL );

	CloseHandle( h );
	if( dwSize == 0 || dwRead == 0 ) return FALSE;

	// check
#ifdef _WIN32_WCE
	char *a = wce_WToA(szFileName);
	bool rc = z.Check( a, dwSize );
	LocalFree( a );
#else
	bool rc = z.Check( szFileName, dwSize );
#endif

	CTool::FreeWorkSpace();

	if( rc == false ) return FALSE;
	else			  return TRUE;
}

//-------------------------------------------------------------------
// DllMain
BOOL APIENTRY
DllMain( HANDLE h, DWORD dw, LPVOID p)
{
    switch( dw ) {
    case DLL_PROCESS_ATTACH:
//		g_hInst = (HINSTANCE)h;
	break;
    case DLL_THREAD_ATTACH:
    break;
    case DLL_THREAD_DETACH:
    break;
    case DLL_PROCESS_DETACH:
    break;
    }
	return TRUE ;
}

int getWildCards(const TCHAR* t, char **argv, int startNum)
{
	int n=startNum;
	TCHAR* p;
	HANDLE h;
	WIN32_FIND_DATA fd;
	TCHAR *tmp, *tmp2, *tmpSubDir;
	TCHAR tmpExt[10]={0};
	int len = 0;
	int i=0;

	tmp  = (TCHAR*)LocalAlloc(LPTR, (MAX_PATH+1)*sizeof(TCHAR));
	tmp2 = (TCHAR*)LocalAlloc(LPTR, (MAX_PATH+1)*sizeof(TCHAR));
	tmpSubDir = (TCHAR*)LocalAlloc(LPTR,
		(MAX_PATH+1)*sizeof(TCHAR));

	// そうでないなら、条件を他に保存して*.* に置き換え
	len = _tcslen(t);
	if( t[len-1]==_T('\"') ) len--;

	// 末尾が *.* なら、tmp にそのまま保存
	// tmp2 に、ワイルドカードを取り除いた文字列を入れる
	if(t[len-3]==_T('*') && t[len-2]==_T('.') && t[len-1]==_T('*')){
		_tcscpy(tmp, t);
		_tcsncpy(tmp2, t, len-3);
	}else{
		// *.txt のような形だと仮定する
		_tcscpy(tmp, t);
		p = _tcschr(tmp, _T('*'));
		*p = _T('\0');
		_tcscpy(tmpExt, p+2); // 拡張子を保存
		_tcscpy(tmp2, tmp);
		_tcscat(tmp, _T("*.*")); // ワイルドカードに置き換え
	}
/*
	// ディレクトリ名は先に登録しておく
	for(i=3; i<n; i++){
		static TCHAR t[MAX_PATH+1];
#ifdef _WIN32_WCE
		wce_AToW2(argv[i], t);
#else
		_tcscpy(t, argv[i]);
#endif
		// 同じのがあったらスキップ
		if(0==_tcscmp(tmp2, t))
			break;
	}
	// 同じのがなかったらここで登録
	if( i == n ){
#ifdef _WIN32_WCE
		argv[n++] = wce_WToA(tmp2);
#else
		argv[n] = (char*)LocalAlloc(LPTR, _tcslen(tmp2)+1);
		_tcscpy(argv[n++], tmp2);
#endif
	}
*/
	h = FindFirstFile(tmp, &fd);

	if(h == INVALID_HANDLE_VALUE){
		return startNum;
	}

	do {
		if(0==_tcscmp(fd.cFileName, _T(".")) ||
			0==_tcscmp(fd.cFileName, _T("..")) )
			continue;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
			wsprintf(tmpSubDir, _T("%s%s\\*.*"),
				tmp2, fd.cFileName);
			n = getWildCards(tmpSubDir, argv, n);
			continue;
		}

		len = _tcslen(fd.cFileName);
		if(tmpExt[0]==0 || 
			(tmpExt[0]==fd.cFileName[len-3] && 
			tmpExt[1]==fd.cFileName[len-2] &&
			tmpExt[2]==fd.cFileName[len-1]) ){
			// 拡張子が指定されていないか、
			// 指定された拡張子と一致すれば登録
			_tcscpy(tmp, fd.cFileName);
			wsprintf(tmp, _T("%s%s"), tmp2, fd.cFileName);
#ifdef _WIN32_WCE
			// CE 用処理
			argv[n++] = wce_WToA(tmp);
#else		// PC 用処理
			argv[n] = (char*)LocalAlloc(LPTR, _tcslen(tmp)+1);
			_tcscpy(argv[n++], tmp);
#endif
		}
//		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

	} while (FindNextFile(h, &fd));	// 次のファイルを探す

	FindClose(h);

	LocalFree(tmp); LocalFree(tmp2);
	LocalFree(tmpSubDir);

	return n;
}
