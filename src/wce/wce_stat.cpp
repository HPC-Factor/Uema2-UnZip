//
// wce_stat.cpp   _stati64 ŠÖ”
//////////////////////////////////////////////////////

#include "wcedef.h"

extern wchar_t *wce_AToW(const char *a);

//int _stati64(const char *filename, struct _stati64 *stat)
//int _stati64(const char *filename, struct _stat *stat)

int _stat(const char *filename, struct _stat *stat)
{
	static wchar_t w_filename[MAX_PATH*2];
	DWORD dwAttribute;
	HANDLE h;
	DWORD dwSizeLow=0, dwSizeHigh=0, dwError=0;
//	FILETIME ctime, atime, mtime;
	WIN32_FIND_DATAW fd;

	wce_AToW2(filename, w_filename);

	dwAttribute = GetFileAttributesW(w_filename);	
	if(dwAttribute==0xFFFFFFFF) return -1;

	stat->st_mode = 0;
	if((dwAttribute & FILE_ATTRIBUTE_DIRECTORY) != 0)
		stat->st_mode += S_IFDIR;
	else
		stat->st_mode += S_IFREG;

	h = FindFirstFileW(w_filename, &fd);
	if(h == INVALID_HANDLE_VALUE){
		if(w_filename[wcslen(w_filename)-1]	== L'\\'){
			w_filename[wcslen(w_filename)-1] = L'\0';
			h = FindFirstFileW(w_filename, &fd);
			if(h == INVALID_HANDLE_VALUE) return 0;
		}else{
			return 0;
		}
	}
	// FILETIME -> time_t •ÏŠ·‚ª•K—vB
	stat->st_atime = FILETIMETotime_t(&fd.ftLastAccessTime);
    stat->st_mtime = FILETIMETotime_t(&fd.ftLastWriteTime);
    stat->st_ctime = FILETIMETotime_t(&fd.ftCreationTime);
	stat->st_size  = fd.nFileSizeLow;

	FindClose( h );
	return 0;
}

/*
int _wstati64(const wchar_t *, struct _stati64 *)
{
	return -1;
}
*/

time_t FILETIMETotime_t(FILETIME* pft)
{
	__int64 time, highTime;

	time = pft->dwLowDateTime;
	highTime = (__int64)pft->dwHighDateTime << 32;
	highTime -= 116444736000000000;
	time += highTime;

	if(time < 0) return 0;
	else         return (time_t)(time/10000000);
}


