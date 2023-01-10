
#include "decMain.h"
#include "xacr\ZipTool.h"
#include "unzip.h"
#include "user_string.h"			//ADD TODA

BOOL checkArchive( char* name );
BOOL deleteArchive( const char* arcname, const char* fname );
BOOL g_bForce = FALSE;
extern void backupArchive(const char* arcname);

// deleteArchive 用 global 変数
struct Header
{
	char* p;
	long  size;
};
struct Header pCE={0}, pC={0};

BOOL backupCentralHeader(HANDLE h);
BOOL findCentralEndHeader(HANDLE h);
void findCentralHeader(HANDLE h, int num);
int  deleteLocalFileData(HANDLE hArc, HANDLE hTmp, 
						 const char* fname);
BOOL checkLocalHeader(HANDLE hArc, const char* fname);
void restoreCentralHeader(HANDLE h, const char* fname);


//--------------------------------------------------------
int decodeMain(int argc, char *argv[])
{
	CZipTool z;
	char temp[64];

	char *p = argv[1];
	if(*p == '-') p++;
	if(*p == 'q') p++;

	if( NULL!=strchr(p, 'f')) g_bForce = TRUE;
	if( NULL!=strchr(p, 'b')) backupArchive(argv[2]);

	switch( *p ){

	case 't':
		return !checkArchive(argv[2]);

	case 'l':
	{
		if(FALSE==checkArchive(argv[2]))
		{
			//MOD START TODA
			sprintf(temp, IDS_FMT_FILE_ERROR1, argv[2]);
			//MOD END
			printfToBuffer(temp);
			return 1;
		}
		z.List( argv[2] );
	}
		break;

	case 'd':
		{for( int i=0; i<argc-3; i++ )
		{
			if( FALSE==deleteArchive(argv[2], argv[3+i]) )
			{
				//MOD START TODA
				sprintf(temp, IDS_FMT_FILE_ERROR1, argv[2]);
				//MOD END
				printfToBuffer(temp);
				return 1;
			}
		}}
		break;

	case 'x':
	case 'e':
	{
		if(FALSE==checkArchive(argv[2])){
			//MOD START TODA
			sprintf(temp, IDS_FMT_FILE_ERROR1, argv[2]);
			//MOD END
			printfToBuffer(temp);
			return 1;
		}

		CTool::AllocWorkSpace();

		// 解凍
		char lpDir[MAX_PATH+1]={0};
		char **extname=NULL;
		int extnum = 0;
		if(argc>=4){
			// base_directory
			if( argv[3][strlen(argv[3])-1] == '\\' ){
				// argv[4] からが解凍対象
				strcpy(lpDir, argv[3]);
				extnum  = argc - 4;
				extname = argv + 4;
			}else{
				// argv[3] からが解凍対象
#ifdef _WIN32_WCE
				strcpy(lpDir, "\\");
#endif
				extnum  = argc - 3;
				extname = argv + 3;
			}
		}

		int ret;
		if( 0!= (ret = z.Extract( argv[2], lpDir, 
			(const char**)extname, extnum)) )
		{
			//MOD START TODA
			sprintf(temp, IDS_FMT_FILE_ERROR2, argv[2]);
			//MOD END
			printfToBuffer(temp);
			return ret;
		}
	}
		break;

	} //switch( *p )

	return 0;
}

//-------------------------------------------------------
BOOL checkArchive( char* name )
{
#ifdef _WIN32_WCE
	TCHAR *t_name = wce_AToW( name );
	BOOL rc = UnZipCheckArchive( t_name, 0 );
	LocalFree( t_name );
#else
	BOOL rc = UnZipCheckArchive( name, 0 );
#endif
	return rc;
}

//-------------------------------------------------------
BOOL deleteArchive(const char* arcname,
				   const char* fname)
{
	// char -> wchar_t
	const wchar_t tmpname[] = L"unziptmp";
	wchar_t* tarcname = (wchar_t*)LocalAlloc( LPTR, 
		( strlen(arcname)+1 )*sizeof(wchar_t) );
	wce_AToW2( arcname, tarcname );

	// 元ファイルと一時ファイルを開いて
	HANDLE hArc = CreateFileW(tarcname, GENERIC_READ, 0, NULL,
		OPEN_EXISTING, 0, NULL);
	if( hArc==INVALID_HANDLE_VALUE ) 
		return FALSE;
	HANDLE hTmp = CreateFileW(tmpname, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, 0, NULL);
	if( hTmp==INVALID_HANDLE_VALUE )
	{ CloseHandle(hArc); return FALSE; }

	// PK12,PK56 を backup
	if( !backupCentralHeader(hArc) )
	{
		// 関数失敗
		CloseHandle(hTmp); CloseHandle(hArc);
		LocalFree( tarcname );
		return FALSE;
	}

	// PK34 を探して、該当ファイル部を削除
	if( 0==deleteLocalFileData(hArc, hTmp, fname) )
	{
		// 0 が返ったら、すべてのファイルを削除
		CloseHandle(hTmp); CloseHandle(hArc);
		DeleteFileW(tarcname); DeleteFileW(tmpname);
		LocalFree( tarcname );
		return TRUE;
	}

	// PK12 から該当部分を削除してファイル書き込み
	// PK56 の内容を書き換えてファイル書き込み
	restoreCentralHeader(hTmp, fname);

	// ファイルハンドル閉じ
	CloseHandle( hArc ); CloseHandle( hTmp );

	// 最後に元ファイルを消し、一時ファイルを
	// 元ファイルの名前に変更
	DeleteFileW(tarcname);
	MoveFileW( tmpname, tarcname );

	LocalFree( pC.p );
	LocalFree( pCE.p );
	LocalFree( tarcname );

	return TRUE;
}

//-------------------------------------------------------
BOOL backupCentralHeader(HANDLE h)
{
	// PK56 を backup
	if( !findCentralEndHeader(h) ) return FALSE;

	short* sp = (short*)pCE.p;

	// PK12 を backup
	findCentralHeader(h, sp[4]);

	return TRUE;
}

//-------------------------------------------------------
BOOL findCentralEndHeader(HANDLE h)
{
	int size = 0, flag = 0;
	char c;
	DWORD d, fp;

	while(1)
	{
		size++;
		fp = SetFilePointer(h,-size,NULL,FILE_END);
		ReadFile(h, &c, 1, &d, NULL);
		if( c==0x06 )      flag |= 1;
		else if( c==0x05 ) flag |= 2;
		else if( c=='K' )  flag |= 4;
		else if( c=='P' )  flag |= 8;
		else               flag = 0;

		if( flag==0xF ) break;
		if( fp==-1 ) return FALSE;
	}

	SetFilePointer(h,-1,NULL,FILE_CURRENT);
	pCE.p = (char*)LocalAlloc( LPTR, size );
	pCE.size = size;
	ReadFile(h, pCE.p, size, &d, NULL);
	return TRUE;
}

//-------------------------------------------------------
void findCentralHeader(HANDLE h, int num)
{
	int size = pCE.size, flag = 0;
	char c;
	DWORD d;

	for( int i=0; i<num; i++ )
	{
		flag=0;
		while(1)
		{
			size++;
			// 1 つづつポインタをずらして読み込み
			SetFilePointer(h,-size,NULL,FILE_END);
			ReadFile(h, &c, 1, &d, NULL);
			if( c==0x02 )      flag |= 1;
			else if( c==0x01 ) flag |= 2;
			else if( c=='K' )  flag |= 4;
			else if( c=='P' )  flag |= 8;

			if( flag==0xF ) break;
		}
	}

	// 読み込み開始位置が決まったら
	// 領域確保してコピー
	SetFilePointer(h,-1,NULL,FILE_CURRENT);
	pC.size = size-pCE.size;
	pC.p = (char*)LocalAlloc( LPTR, pC.size );
	ReadFile( h, pC.p, pC.size, &d, NULL);
}

//-------------------------------------------------------
int deleteLocalFileData(HANDLE hArc, HANDLE hTmp, 
						 const char* fname)
{
	DWORD d;
	BOOL  b = FALSE, bb=FALSE;
	char c;
	int flag=0;

	// まずファイルポインタを先頭にして
	SetFilePointer( hArc, 0, NULL, FILE_BEGIN );
	DWORD size = GetFileSize( hArc, &d );
	size -= pCE.size + pC.size;

	for(DWORD i=0; i<size; i++ )
	{
		// 1 バイト読んで
		ReadFile( hArc, &c, 1, &d, NULL );

		// PK34 だったら 26 バイト読んで、
		// ファイル名のサイズを取得して
		// ファイル名を読んで
		if( c==0x03 )      flag|=1; 
		else if( c==0x04 ) flag|=2;
		else if( c=='P' )  flag|=4;
		else if( c=='K' )  flag|=8;
		else               flag=0;

		if( flag==0x0F )
		{
			b = checkLocalHeader(hArc, fname);
			// TRUE だったら、PK3 を戻さなきゃならん
			if( b != bb )
			{
				if( b )
				{
					// false -> true の場合は、
					// 前の PK3 までがすでに Write されているので
					// その部分を削除するために 3 つ戻す
					// また、格納ファイル数が 1 で、それを消そうとした場合は
					// ファイルごと削除する必要あり
					short* sp = (short*)pCE.p;
					if( sp[4]==1 ) return 0;
					SetFilePointer( hTmp, -3, NULL, FILE_CURRENT );
				}
				else
				{
					// true -> false の場合は、
					// 前の PK3 がスキップされているので、
					// その部分を書いてやる必要アリ
					const char ar[3]={0x50, 0x4b, 0x03};
					WriteFile( hTmp, ar, 3, &d, NULL );
				}
				bb=b;
			}
			flag=0;
		}

		// fname と一致しなかったら以下をファイルに書く
		if( !b ) WriteFile( hTmp, &c, 1, &d, NULL );
	}

	return 1;
}

//-------------------------------------------------------
BOOL checkLocalHeader(HANDLE hArc, const char* fname)
{
	short s[13];
	DWORD d;

	// 26 バイト読む
	ReadFile( hArc, s, 26, &d, NULL );

	// ファイル名の長さを取得
	int len = s[11];

	// バッファを確保してファイル名取得
	char* p = (char*)LocalAlloc( LPTR, len+1 );
	ReadFile( hArc, p, len, &d, NULL );

	// ファイルポインタを元に戻しておく
	SetFilePointer( hArc, -(26+len), NULL, FILE_CURRENT );

	// 文字列比較
	int cmp = strnicmp(fname, p, len);
	LocalFree(p);

	if( cmp==0 ) return TRUE;
	else         return FALSE;
}

//-------------------------------------------------------
void restoreCentralHeader(HANDLE h, const char* fname)
{
	char*   p = pC.p;
	short* sp = (short*)p;
	int offset= 0, headsize, delsize, totalsize=0;
	DWORD d;

	// PK12 から fname 部分を削除して書込み
	while(1)
	{
		headsize = 46+sp[14]+sp[15]+sp[16];
		if( 0!=strnicmp( &p[46], fname, sp[14] ) )
		{
			WriteFile(h, p, headsize, &d, NULL );
			totalsize += headsize;
		}
		else
		{
			delsize = headsize;
		}
		p += headsize;
		sp += (headsize)/2;
		offset += headsize;
		if(offset >= pC.size ) break;
	}

	// PK56 から fname に関する情報を削除して書込み
	p       = pCE.p;
	sp      = (short*)p;
	int *ip = (int*)p;
	// 格納ファイル数
	sp[4]--; sp[5]--;
	// ヘッダサイズ
	ip[3] -= delsize;
	// 先頭から CentralHeader までのサイズ
	DWORD fsize = GetFileSize( h, NULL ) - totalsize - pCE.size;
	ip[4] = fsize;
	WriteFile(h, pCE.p, pCE.size, &d, NULL );
}
