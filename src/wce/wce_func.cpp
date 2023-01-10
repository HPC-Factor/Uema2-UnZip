
#include "wcedef.h"

BOOL DosDateTimeToSystemTime(WORD date, WORD time, SYSTEMTIME* s)
{
	if(date==0) return FALSE;
	s->wYear =((date & 0xFE00) >> 9) + 1980;
	s->wMonth= (date & 0x1E0)  >> 5;
	s->wDay  =  date & 0x1F;

	s->wHour   = (time & 0xF800) >> 11;
	s->wMinute = (time & 0x7E0) >> 5;
	s->wSecond = (time & 0x1F) * 2;
	
	s->wDayOfWeek = 0;
	s->wMilliseconds = 0;

	return TRUE;
}

#ifdef _WIN32_WCE
BOOL FileTimeToDosDateTime(CONST FILETIME *pF,
                           LPWORD lpDate,LPWORD lpTime)
{
	SYSTEMTIME s;

	if(0==FileTimeToSystemTime(pF, &s))
		return 0;

	*lpDate = 0;
	*lpDate += s.wDay;
	*lpDate += s.wMonth << 5;
	*lpDate += (s.wYear - 1980) << 9;

	*lpTime = 0;
	*lpTime += s.wSecond/2;
	*lpTime += s.wMinute << 5;
	*lpTime += s.wHour << 11;

	return 1;
}
#endif

void printfToBuffer(const char* a)
{
	TCHAR *t;
	static int init = 0;
	
	if(g_outBuff==NULL) return;

#ifdef _WIN32_WCE
	t = wce_AToW(a);
#else
	t = (TCHAR*)a;
#endif

	if(init==0){
		_tcscpy(g_outBuff, t);
		init = 1;
	}else{
		_tcscat(g_outBuff, t);
	}

#ifdef _WIN32_WCE
	LocalFree(t);
#endif

}

#ifdef _WIN32_WCE
char *CharNextA(const char* a)
{
	char *p=(char *)a;
	if(TRUE==IsDBCSLeadByteEx(CP_ACP, (BYTE)*a))
		p+=2;
	else
		p++;

	return p;
}

int _stricmp( const char *s1, const char *s2 )
{
//	int cmpResult;
    const char *p1 = s1;
	const char *p2 = s2;

	// 要するに、大文字小文字の区別がない strcmp だ。
    while( *p1!='\0' && *p2!='\0' ){
		if( *p1 != *p2 ){
			// p1 が大文字だったら、小文字に置き換えて計算
			if( 0x41 <= *p1 && *p1 <= 0x5A ){
				if( *p1+0x20 != *p2 )
					return 1;
			}
			// p1 が小文字だったら、大文字に置き換えて計算
			else if( 0x61 <= *p1 && *p1 <= 0x7A ){
				if( *p1-0x20 != *p2 )
					return -1;
			}
		}
		p1 = CharNextA(p1); p2 = CharNextA(p2);
		if( *p1 == '\0' && *p2 != '\0' ) return 1;
		if( *p1 != '\0' && *p2 == '\0' ) return -1;
	}
	return 0;
}

int _strnicmp( const char *s1, const char *s2, size_t size )
{
//	int cmpResult;
    const char *p1 = s1;
	const char *p2 = s2;
	size_t i;

	// 要するに、大文字小文字の区別がない strncmp だ。
//	while( p1!='\0' && p2!='\0' ){
	for( i=0; i<size; i++ )
	{
		if( *p1 != *p2 ){
			// p1 が大文字だったら、小文字に置き換えて計算
			if( 0x41 <= *p1 && *p1 <= 0x5A ){
				if( *p1+0x20 != *p2 )
					return 1;
			}
			// p1 が小文字だったら、大文字に置き換えて計算
			else if( 0x61 <= *p1 && *p1 <= 0x7A ){
				if( *p1-0x20 != *p2 )
					return -1;
			}
		}
		p1 = CharNextA(p1); p2 = CharNextA(p2);
		if( *p1 == '\0' && *p2 != '\0' ) return 1;
		if( *p1 != '\0' && *p2 == '\0' ) return -1;
	}
	return 0;
}

#endif

