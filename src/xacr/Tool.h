#ifndef AFX_TOOL_H__57AAB1DD_886F_4D25_860C_2AA917B02AC1__INCLUDED_
#define AFX_TOOL_H__57AAB1DD_886F_4D25_860C_2AA917B02AC1__INCLUDED_

//#include "kiutil.h"
//#define XACR_BUFSIZE (0x100000)
// ↑ must be bigger than 0x100000 !! ( for RAR )
#define XACR_BUFSIZE (0x20000)
// zip だけなんで、200kb もあれば十分かなーとか

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
//#include "wce\wcedef.h"

// WIN( 0.1 millisec from 1601.1.1 )
void timeSet( const char* fname, FILETIME* pft );
// DOS( 0-4:day 5-8:month 9-15:year+1980, 0-4:sec/2 5-10:min 11-15:hour )
void timeSet( const char* fname, WORD date, WORD time );
// UNIX( sec from 1970.1.1 )
void timeSet( const char* fname, DWORD sec );

void pathInit();

// 相対パスを与えると [不正文字除去],[複数階層makeDir] を行う
//static char* pathMake( char* path );
char* pathMake( char* path );

// 絶対パスを与えると [複数階層makeDir]
void pathMakeAbs( char* path );


class CTool
{
public:

////////////////////////////////////////////////
// 派生クラス作成についてMEMO
//
// GetPassword
// 　パスワードが必要になったらこれを呼ぶこと
// 　呼ぶ度にダイアログを出してユーザに入力させる
// common_buf
// 　作業用に自由に使ってよし。
// 　XACR_BUFSIZE byte 確保されているはず。
// 　
////////////////////////////////////////////////
//
// コンストラクタ
// 　CTool( ルーチン名 )を必ず呼び出すこと
//
// デストラクタ
// 　自由に
//
// IsType
// 　拡張子から対応書庫かどうか判定。ただし、拡張子
// 　以外のマトモな判定手段を持たないルーチンは
// 　ここでは必ず false を返さなくてはならない
//
// Check
// 　ファイル名, ファイルサイズ と、common_buf に入っている
// 　ファイル先頭の min(XACR_BUFSIZE,fsize) bytes を使って厳密に判定
// 　common_buf はこの時書き換え不可！
//
// Extract
// 　書庫"fname"をディレクトリ"ddir"(=カレント)に展開
//
////////////////////////////////////////////////

// 仮想関数 ////////////////

	virtual bool IsType( const char* ext ){return false;}
	virtual bool Check( const char* fname, unsigned long fsize ) = 0;
	virtual int  Extract( const char* fname, const char* ddir,
		const char **extname=NULL, int extnum=0) = 0;

// コンストラクタ＆デストラクタ ////

protected:
	CTool( const char* name )
		: m_Name(name), m_pNext(NULL), m_pPass(NULL){}
public:
	virtual ~CTool()
		{delete m_pNext;	delete [] m_pPass;}

// 作業用バッファ ///////

public:
	static void AllocWorkSpace()
		{common_buf = new unsigned char[XACR_BUFSIZE];}
	static void FreeWorkSpace()
		{delete [] common_buf; common_buf=NULL;}

	static unsigned char* common_buf;

// ルーチン名 ///////////

public:
	const char* GetRoutineName()
		{return m_Name;}
private:
	const char* m_Name;

// 簡易リスト ///////////

public:
	CTool* GetNext()
		{return m_pNext;}
	void AddTail( CTool* p )
		{
			if( !m_pNext )	m_pNext=p;
			else			m_pNext->AddTail(p);
		}
private:
	CTool* m_pNext;

// パスワード ///////////

protected:
	char* GetPassword();
private:
	char* m_pPass;
	static BOOL CALLBACK PassProc( HWND dlg,UINT msg,WPARAM wp,LPARAM lp );

////////////////////////
};

#endif
