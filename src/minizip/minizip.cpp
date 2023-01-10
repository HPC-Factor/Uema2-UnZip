
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
//#include <time.h>
//#include <errno.h>
//#include <fcntl.h>

#ifdef unix
# include <unistd.h>
# include <utime.h>
# include <sys/types.h>
# include <sys/stat.h>
#else
//# include <direct.h>
//# include <io.h>
#endif

#include "mnzip.h"
#include "..\wce\wcedef.h"
#include "..\unzipdlg.h"

int isFileExist(const char* fname);
void backupArchive(const char* arcname);
int g_err = ZIP_OK+1;

// dummy
//TCHAR g_outBuff[64*1024];
//char *base_directory;

#define WRITEBUFFERSIZE (16384)
#define MAXFILENAME (256)

#ifdef WIN32
uLong filetime(
    const char *f,  /* name of file to get info on */
    tm_zip *tmzip,  /* return value: access, modific. and creation times */
    uLong *dt       /* dostime */
    )
{
  int ret = 0;
  {
      FILETIME ftLocal;
      HANDLE hFind;
      WIN32_FIND_DATA  ff32;
	  static TCHAR tf[MAX_PATH*2]={0};

#ifdef _WIN32_WCE
	  wce_AToW2(f, tf);
#else
	  strcpy(tf, f);
#endif

      hFind = FindFirstFile(tf,&ff32);
      if (hFind != INVALID_HANDLE_VALUE)
      {
        FileTimeToLocalFileTime(&(ff32.ftLastWriteTime),&ftLocal);
        FileTimeToDosDateTime(&ftLocal,((LPWORD)dt)+1,((LPWORD)dt)+0);
        FindClose(hFind);
        ret = 1;
      }
  }
  return ret;
}
#else
#ifdef unix
uLong filetime(
    char *f,        /* name of file to get info on */
    tm_zip *tmzip,  /* return value: access, modific. and creation times */
    uLong *dt       /* dostime */
    )
{
  int ret=0;
  struct stat s;        /* results of stat() */
  struct tm* filedate;
  time_t tm_t=0;
  
  if (strcmp(f,"-")!=0)
  {
    char name[MAXFILENAME];
    int len = strlen(f);
    strcpy(name, f);
    if (name[len - 1] == '/')
      name[len - 1] = '\0';
    /* not all systems allow stat'ing a file with / appended */
    if (stat(name,&s)==0)
    {
      tm_t = s.st_mtime;
      ret = 1;
    }
  }
  filedate = localtime(&tm_t);

  tmzip->tm_sec  = filedate->tm_sec;
  tmzip->tm_min  = filedate->tm_min;
  tmzip->tm_hour = filedate->tm_hour;
  tmzip->tm_mday = filedate->tm_mday;
  tmzip->tm_mon  = filedate->tm_mon ;
  tmzip->tm_year = filedate->tm_year;

  return ret;
}
#else
uLong filetime(
    const char *f, /* name of file to get info on */
    tm_zip *tmzip, /* return value: access, modific. and creation times */
    uLong *dt      /* dostime */
	)
{
    return 0;
}
#endif
#endif


int check_exist_file(
    const char* filename)
{
	FILE* ftestexist;
    int ret = 1;
	ftestexist = fopen(filename,"rb");
	if (ftestexist==NULL)
        ret = 0;
    else
        fclose(ftestexist);
    return ret;
}

void do_banner()
{
	printfToBuffer(
		"MiniZip 0.15, demo of zLib + Zip package written by Gilles Vollant\n");
	printfToBuffer(
		"more info at http://wwww.winimage/zLibDll/unzip.htm\n\n");
}

void do_help()
{	
	/* o = "O"verwrite */
	printfToBuffer(
		"Usage : minizip [-o] file.zip [files_to_add]\n\n") ;
}

//int main(int argc, char *argv[])
int zipMain(int argc, char *argv[])
{
	int i;
	int opt_overwrite=0;
    int opt_compress_level=Z_DEFAULT_COMPRESSION;
    int zipfilenamearg = 0;
	char filename_try[MAXFILENAME];
    int zipok;
    int err=0;
    int size_buf=0;
    void* buf=NULL,


	do_banner();
	if (argc==1)
	{
		do_help();
//		exit(0);
        return 0;
	}
	else
	{
		for (i=1;i<argc;i++)
		{
			if ((*argv[i])=='-')
			{
				const char *p=argv[i]+1;
				opt_overwrite = 1;

				while ((*p)!='\0')
				{
					char c=*(p++);
//					if ((c=='o') || (c=='O'))
//						opt_overwrite = 1;
                    if ( c=='o' || ((*p>='0') && (*p<='9')))
						opt_compress_level = *p-'0';
					else if( c=='b' )
						backupArchive(argv[2]);
				}
			}
			else
				if (zipfilenamearg == 0)
                    zipfilenamearg = i ;
		}
	}

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf==NULL)
    {
        printfToBuffer("Error allocating memory\n");
        return ZIP_INTERNALERROR;
    }

	if (zipfilenamearg==0)
        zipok=0;
    else
	{
        int i,len;
        int dot_found=0;

        zipok = 1 ;
		strcpy(filename_try,argv[zipfilenamearg]);
        len=strlen(filename_try);
        for (i=0;i<len;i++)
            if (filename_try[i]=='.')
                dot_found=1;

        if (dot_found==0)
            strcat(filename_try,".zip");

        if (opt_overwrite==0)
            if (check_exist_file(filename_try)!=0)
			{
                char rep;
				do
				{
					char answer[128];
					// 上書き確認
					printf("The file %s exist. Overwrite ? [y]es, [n]o : ",filename_try);
					scanf("%1s",answer);
					rep = answer[0] ;
					if ((rep>='a') && (rep<='z'))
						rep -= 0x20;
				}
				while ((rep!='Y') && (rep!='N'));
                if (rep=='N')
                    zipok = 0;
			}
    }

    if (zipok==1)
    {
  		static char temp[128];
        zipFile zf;
        int errclose;
		int file_exist = isFileExist(filename_try);

        zf = zipOpen( filename_try, file_exist );
        if (zf == NULL)
        {
			sprintf(temp, "error opening %s\n",filename_try);
			printfToBuffer(temp);
            err= ZIP_ERRNO;
        }
        else
		{
			sprintf(temp, "creating %s\n",filename_try);
			printfToBuffer(temp);
			dlgSetArchiveName(filename_try); //uema2.
		}

		if( argv[3][strlen(argv[3])-1] == '\\'){
			base_directory = argv[3];
			zipfilenamearg ++;
		}

        for (i=zipfilenamearg+1;(i<argc) && (err==ZIP_OK);i++)
        {
            if (((*(argv[i]))!='-') && ((*(argv[i]))!='/'))
            {
                FILE * fin=NULL;
                int size_read;
                const char* filenameinzip = argv[i];
                zip_fileinfo zi;

	            zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour = 
	            zi.tmz_date.tm_mday = zi.tmz_date.tm_min = zi.tmz_date.tm_year = 0;
	            zi.dosDate = 0;
	            zi.internal_fa = 0;
	            zi.external_fa = 0;
	            filetime(filenameinzip,&zi.tmz_date,&zi.dosDate);

				// 最後の引数が圧縮レベル
	            err = zipOpenNewFileInZip(zf,filenameinzip,&zi,
	                                 NULL,0,NULL,0,NULL /* comment*/,
	                                 (opt_compress_level != 0) ? Z_DEFLATED : 0,
	                                 opt_compress_level);

				if(err != ZIP_OK){
					sprintf(temp, "error in opening %s in zipfile\n",filenameinzip);
					printfToBuffer(temp);
				}
		        else
		        {
					fin = fopen(filenameinzip,"rb");
					if (fin==NULL)
					{
					    err=ZIP_ERRNO;
						sprintf(temp, "error in opening %s for reading\n",filenameinzip);
						printfToBuffer(temp);
					}
				}

                if (err == ZIP_OK)
				{
					//-----------------------------uema2.
					struct _stat st={0};
					unsigned long totalSize = 0;
					dlgSetFileName(filenameinzip);
					_stat(filenameinzip, &st);
					dlgSetFileSize( st.st_size, 0 );
					progressSetRange( 0, st.st_size/size_buf +1 );
					progressSetPos( 0 );
					//----------------------------------
                    do
                    {
                        err = ZIP_OK;
                        size_read = fread(buf,1,size_buf,fin);
                        if (size_read < size_buf)
                            if (feof(fin)==0)
                        {
							sprintf(temp, "error in reading %s\n",filenameinzip);
							printfToBuffer(temp);
                            err = ZIP_ERRNO;
                        }

                        if (size_read>0)
                        {
							totalSize += size_read;          //uema2.
							dlgSetFileSize( -1, totalSize ); //uema2.
						    progressGetAStep(); //uema2.
                            err = zipWriteInFileInZip (zf,buf,size_read);
                            if (err<0)
                            {
								sprintf(temp, "error in writing %s in the zipfile\n",
                                                 filenameinzip);
								printfToBuffer(temp);
                            }

                        }
                    } while ((err == ZIP_OK) && (size_read>0));

				} //if

                if( fin!=NULL ) fclose(fin);
                if (err<0)
                    err=ZIP_ERRNO;
                else
                {
					g_err = ZIP_OK;
                    err = zipCloseFileInZip(zf);
                    if (err!=ZIP_OK){
						sprintf(temp, "error in closing %s in the zipfile\n",
                                    filenameinzip);
						printfToBuffer(temp);
					}
                }
            }
        }
		// 以前に追加に成功しているなら実行
//		errclose = zipClose(zf,NULL);
		errclose = zipClose2(zf,NULL,file_exist,g_err);
        if (errclose != ZIP_OK){
			sprintf(temp, "error in closing %s\n",filename_try);
			printfToBuffer(temp);
		}
   }

    free(buf);
//    exit(0);
	return 0;  /* to avoid warning */
}

int isFileExist(const char* fname)
{
	int length = wce_AToW2( fname, NULL );
	wchar_t *t = (wchar_t*)::LocalAlloc( LPTR, 
		(length+1)*sizeof(wchar_t) );
	wce_AToW2( fname, t );

	HANDLE h = ::CreateFileW( t, GENERIC_READ, 
		FILE_SHARE_READ, NULL, OPEN_EXISTING,
		0, 0 );

	if( h == INVALID_HANDLE_VALUE )
		return 0;
	
	::LocalFree( t );
	::CloseHandle( h );
	return 1;
}

void backupArchive( const char* arcname )
{
	int i=0, len = strlen(arcname);

	if( !isFileExist( arcname ) ) return;

	wchar_t *tarcname = (wchar_t*)LocalAlloc( 
		LPTR, (len+1)*sizeof(wchar_t) );
	wce_AToW2( arcname, tarcname );
	wchar_t *bkname = (wchar_t*)LocalAlloc(
		LPTR, (len+5)*sizeof(wchar_t) );

	// bkname に .bak のファイル名を作成
	for( i=0; i<len; i++ )
	{
		if( tarcname[len-1-i]==L'.' )
		{
			wcsncpy( bkname, tarcname, len-i );			
			bkname[len-i] = L'\0';
			wcscat( bkname, L"bak" );
			break;
		}
	}

	// . が見つからなかったら、そのまま .bak をくっつけ
	if( i==len ) wsprintfW( bkname, L"%s.bak", tarcname );

	// 強制上書きでコピー
	CopyFileW( tarcname, bkname, FALSE );
#ifdef _DEBUG
	OutputDebugStringW( tarcname );
	OutputDebugStringW( bkname );
#endif
	LocalFree( tarcname ); LocalFree( bkname );
}

