
//#include "stdafx.h"
#include "ZipTool.h"
#include "..\wce\wcedef.h"
#include "..\unzip.h"
#include "..\unzipdlg.h"
#include "..\zlib/zlib.h"
#include "..\user_string.h"			//ADD TODA

extern BOOL g_bForce;

// List �p
void printHeader();
void printFooter(unsigned long,unsigned long);

// �P�t�@�C���𓀂̂���
bool compareFileName(const char*fname, const char **extname,
					 int extnum);


//-- CZipTool ���� -----------------------

bool CZipTool::Check( const char* fname, unsigned long fsize )
{
	const unsigned char* hdr = CTool::common_buf;
	unsigned long siz = (fsize>XACR_BUFSIZE ? XACR_BUFSIZE : fsize);
	//--------------------------------------------------------------------//

	int i;
	for( i=0; i<(signed)siz-30; i++ )
	{
		if( hdr[i+0] != 'P'  )continue;
		if( hdr[i+1] != 'K'  )continue;
		if( hdr[i+2] != 0x03 )continue;
		if( hdr[i+3] != 0x04 )continue;
		if( hdr[i+8]+(hdr[i+9]<<8) > 12 )continue;
		if( hdr[i+26]==0 && hdr[i+27]==0 )continue;
		break;
	}
	if( (unsigned)i+30>=siz )
		return false;

	// �O�̂��ߓǂݍ���ł݂Ċm���߂�
	if( !(zip=fopen( fname, "rb" )) )
		return false;

	dlgSetArchiveName(fname);

	fseek( zip, i+4, SEEK_SET );
	ZipLocalHeader zhdr;
	bool ans = read_header( &zhdr );
	fclose( zip );

	return ans;
}

int CZipTool::Extract( const char* fname,const char* ddir,
					const char **extname, int extnum)
{
	char password[128] = {'\0'};
	char fullPath[MAX_PATH+1] = {0};
	ZipLocalHeader hdr;

	if( !(zip=fopen(fname,"rb")) )
		return ERROR_ARCHIVE_FILE_OPEN;

	dlgSetArchiveName(fname);
	progressSetRange(0, 1);

	while( doHeader( &hdr,CTool::common_buf ) )
	{
		unsigned long posnext = ftell( zip ) + hdr.csz;

		// ��v������ true
		if( true != compareFileName(hdr.fnm, extname, extnum) ){
			fseek( zip, posnext, SEEK_SET );
			continue;
		}

		sprintf(fullPath, "%s%s", ddir, hdr.fnm);
		char *name = pathMake( fullPath );

		// ���łɑ��݂��邩�ǂ��������Ŋm�F
		struct _stat st;
		if(g_bForce==FALSE && 0 <= _stat(fullPath, &st) &&
			(st.st_mode & S_IFDIR) == 0){
			//Overwrite?
			int rc = createInquireDialog(fullPath, 
				st.st_mtime, hdr.dat, hdr.tim);
			if( rc == 0 ){ // yes
				;
			}else if( rc == 2 ){ // no
				continue;
			}else if( rc == 4 ){ // all
				g_bForce = TRUE;
			}else if( rc == 6 ){ // skip
				//MOD START TODA
				printfToBuffer(IDS_MSG_SKIP);
				//MOD END
				//ADD START TODA
				if(zip) fclose(zip);
				//ADD END
				return ERROR_USER_SKIP;
			}
		}
//		char *name = pathMake( hdr.fnm );
//		if( out=fopen(name,"wb") )
		if( out=fopen(fullPath,"wb") )
		{
			dlgSetFileName(name);
			dlgSetFileSize(hdr.usz, 0);
			progressSetPos( 0 );

			write_init();   // �o��CRC���N���A
			if( hdr.flg&1 ) // �Í�������Ă�̂Ńp�X���[�h���Z�b�g
			{
				if( *password=='\0' || !read_init(password,&hdr) )
				{
					int i;
//					strncpy( password, GetPassword(), sizeof(password)-1 );
					for( i=0; i!=3; i++ )
					{
						strncpy( password, GetPassword(), sizeof(password)-1 );
						if( read_init( password, &hdr ) )
							break;
					}
					if( i==3 ) // �p�X���[�h�s��v
					{
						fclose( out );
#ifdef _WIN32_WCE
						TCHAR *t_name = wce_AToW( name );
						DeleteFile( t_name );
						LocalFree( t_name );
#else
						DeleteFile( name );
#endif
//						break;
						//MOD START TODA
						printfToBuffer(IDS_MSG_PASSWD_NG);
						//MOD END
						//ADD START TODA
						if(zip) fclose(zip);
						//ADD END
						return ERROR_PASSWORD_FILE;
					}
				}
			}
			else // �p�X���[�h���g��Ȃ�
				read_init();

			progressGetAStep(); //uema2.

			// �W�J�I
			switch( hdr.mhd )
			{
			case Stored:	Unstore( hdr.usz, hdr.csz );	break;
			case Deflated:	Inflate( hdr.usz, hdr.csz );	break;
			case Shrunk:	Unshrink( hdr.usz, hdr.csz );	break;
			case Reduced1:	Unreduce( hdr.usz, hdr.csz, 1 );	break;
			case Reduced2:	Unreduce( hdr.usz, hdr.csz, 2 );	break;
			case Reduced3:	Unreduce( hdr.usz, hdr.csz, 3 );	break;
			case Reduced4:	Unreduce( hdr.usz, hdr.csz, 4 );	break;
			case Imploded:	Explode( hdr.usz, hdr.csz,
								 0!=(hdr.flg&0x02), 0!=((hdr.flg)&0x04) );
				break;
			}

			// ���㏈��
			fclose( out );

			dlgSetFileSize(hdr.usz, hdr.usz);

			timeSet( name,
				(WORD)hdr.dat,(WORD)hdr.tim);
		}

		fseek( zip, posnext, SEEK_SET );
	}

	fclose( zip );

	// 0 �Ȃ琬��
	return 0;
}

bool CZipTool::List( const char* fname )
{
	// LHa 1.14f �`���B
	char temp[64] = {0};
	unsigned long totalSize=0, totalFile=0;
	int iType=0;
	char type[]={" STOR DEFL SRNK RED1 RED2 RED3 RED4 IMPL"};
	ZipLocalHeader hdr;
	SYSTEMTIME s={0};

	if( !(zip=fopen(fname,"rb")) )
		return false;

	dlgSetArchiveName(fname);

	//
	printHeader();

	while( doHeader( &hdr,CTool::common_buf ) )
	{
		unsigned long posnext = ftell( zip ) + hdr.csz;

		// hdr.fnm = �t�@�C����
		// hdr.usz = �T�C�Y
		// hdr.tim = �^�C���X�^���v
		// hdr.csz / hdr.usz * 100 = Ratio ?

		//32bitCRC
		sprintf(temp, "%08X", hdr.crc);
		printfToBuffer(temp);
		//TYPE
		switch( hdr.mhd )
		{
		case Stored:	iType=0; break;
		case Deflated:	iType=1; break;
		case Shrunk:	iType=2; break;
		case Reduced1:	iType=3; break;
		case Reduced2:	iType=4; break;
		case Reduced3:	iType=5; break;
		case Reduced4:	iType=6; break;
		case Imploded:	iType=7; break;
		}
		strncpy(temp, &type[iType*5], 5);
		temp[5]='\0';
		printfToBuffer(temp);
		//SIZE
		sprintf(temp, " %8d", hdr.usz);
		printfToBuffer(temp);
		//RETIO
		if( hdr.usz == 0 ){
			if( hdr.fnm[strlen(hdr.fnm)-1]=='/' ||
				hdr.fnm[strlen(hdr.fnm)-1]=='\\')
				strcpy(temp, " ******");
			else
				strcpy(temp, "   0.0%");
		}else{
			sprintf(temp, "  %3.1f%%", (float)hdr.csz / (float)hdr.usz * 100);
		}
		printfToBuffer(temp);
		//STAMP
		DosDateTimeToSystemTime(
			hdr.dat, hdr.tim, &s);
		sprintf(temp, " %d-%02d-%02d %2d:%02d:%02d", s.wYear,
			s.wMonth, s.wDay, s.wHour, s.wMinute, s.wSecond);
		printfToBuffer(temp);
		//NAME
		sprintf(temp, " %s\n", hdr.fnm);
		printfToBuffer(temp);

		fseek( zip, posnext, SEEK_SET );
		totalSize+=hdr.usz;
		totalFile++;
	}

	//
	printFooter(totalFile, totalSize);

	fclose( zip );
	return true;
}

void printHeader()
{
	printfToBuffer("  CRC    TYPE   SIZE    RATIO       STAMP             NAME\n");
	printfToBuffer("-------- ---- -------- ------ ------------------- ----------------------\n");
}

void printFooter(unsigned long totalFile, unsigned long totalSize)
{
	char temp[64];
	printfToBuffer("-------- ---- -------- ------ ------------------- ----------------------\n");
	sprintf(temp,  "  Total      %9d             %9dfiles\n", totalSize, totalFile);
	printfToBuffer(temp);
}

bool compareFileName(const char *fname, const char **extname,
					 int extnum)
{
//	char temp[MAX_PATH+1];

	if(extnum == 0) return true;

	for( int i=0; i<extnum; i++ ){
		if(0==stricmp(fname, extname[i]))
			return true;
	}
	return false;
}

//---- CRC �֌W ---------------------

// make crc table
//static void zip_make_crc_table()
//{
//	for( DWORD c,n=0; n!=256; n++ )
//	{
//		c = n;
//		for( DWORD k=8; k; k-- )
//			c = (c&1) ? ((0xedb88320L)^(c>>1)) : (c>>1);
//		crc_table[n] = c;
//	}
//}

// the result
static DWORD crc_table[256] = {
0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// update crc
#define calc_crc(c,b) (crc_table[(c&0xff)^(b&0xff)]^((c)>>8))

DWORD CZipTool::crc32( DWORD crc, const BYTE* dat, int len )
{
	crc ^= 0xffffffffL;
	while( len-- )
		crc = calc_crc( crc,*dat++ );
	return (crc^0xffffffffL);
}

//-- �p�X���[�h�����֌W ------------------------

int CZipTool::decrypt_byte()
{
	unsigned temp = ((unsigned)(keys[2]&0xffff))|2;
	return (int)(((temp*(temp^1))>>8)&0xff);
}

int CZipTool::update_keys( int c )
{
	keys[0] = calc_crc( keys[0],c );
	keys[1]+= (keys[0]&0xff);
	keys[1] = keys[1]*134775813L + 1;
	keys[2] = calc_crc( keys[2], (keys[1]>>24) );
	return c;
}

#define RAND_HEAD_LEN (12)
bool CZipTool::read_init( const char* pwd, ZipLocalHeader* hdr )
{
	if( pwd == NULL )
	{
		needdec = false;
		return true;
	}

	needdec = true;

	// ���̃p�X���[�h�ŏ�����
	keys[0] = 305419896L;
	keys[1] = 591751049L;
	keys[2] = 878082192L;
	while( *pwd )
		update_keys( *pwd++ );

	// ���̈ʒu���L��
	int x = ftell( zip );

	// password�`�F�b�N���ǂݍ���
	BYTE eh[RAND_HEAD_LEN];
	bool ans = false;
	if( RAND_HEAD_LEN == fread( eh, 1, RAND_HEAD_LEN, zip ) )
	{
		for( int i=0; i!=RAND_HEAD_LEN; i++ )
			zdecode( eh[i] );

		// �t���O�ɂ���āA�`�F�b�N�ɗp����l�͈Ⴄ
		WORD checkerval = ((hdr->flg&8)==8 ? hdr->tim : (WORD)(hdr->crc>>16));

		// ����
		if( (eh[RAND_HEAD_LEN-2] | (eh[RAND_HEAD_LEN-1]<<8)) == checkerval )
			ans = true;
	}

	if( ans ) // �`�F�b�N����ǂ񂾕��A���k�T�C�Y��ύX
		hdr->csz = hdr->csz>RAND_HEAD_LEN ? hdr->csz-RAND_HEAD_LEN : 0;
	else	  // �`�F�b�N���̕��A�߂�
		fseek( zip, x, SEEK_SET );
	return ans;
}

//-- �w�b�_���� -----------------------------------

bool CZipTool::read_header( ZipLocalHeader* hdr,unsigned char* buf )
{
	bool del = (buf==NULL);
	if( buf==NULL ) buf = new unsigned char[65536];

	if( 26 != fread(buf,1,26,zip) )
		{ if(del)delete [] buf; return false; }

	hdr->ver = ((buf[ 0])|(buf[ 1]<<8));
	hdr->flg = ((buf[ 2])|(buf[ 3]<<8));
	hdr->mhd = ((buf[ 4])|(buf[ 5]<<8));
	hdr->tim = ((buf[ 6])|(buf[ 7]<<8));
	hdr->dat = ((buf[ 8])|(buf[ 9]<<8));
	hdr->crc = ((buf[10])|(buf[11]<<8)|(buf[12]<<16)|(buf[13]<<24));
	hdr->csz = ((buf[14])|(buf[15]<<8)|(buf[16]<<16)|(buf[17]<<24));
	hdr->usz = ((buf[18])|(buf[19]<<8)|(buf[20]<<16)|(buf[21]<<24));
	hdr->fnl = ((buf[22])|(buf[23]<<8));
	hdr->exl = ((buf[24])|(buf[25]<<8));

	if( hdr->fnl!=fread(buf,1,hdr->fnl,zip) )
		{ if(del)delete [] buf; return false; }

	int l = (hdr->fnl>MAX_PATH-1)?(MAX_PATH-1):(hdr->fnl);
	memcpy( hdr->fnm, buf, l );
	hdr->fnm[l]='\0';
	char* pp;
	for( pp=hdr->fnm; *pp!=0; pp++ );

	if( hdr->mhd > Deflated || hdr->mhd==Tokenized )
		{ if(del)delete [] buf; return false; }
	if( pp-hdr->fnm != l )
		{ if(del)delete [] buf; return false; }
	if( hdr->exl != fread(buf,1,hdr->exl,zip) )
		{ if(del)delete [] buf; return false; }

	if(del){
		delete [] buf; buf = NULL;
	}
	return true;
}

bool CZipTool::doHeader( ZipLocalHeader* hdr,unsigned char* buf )
{
	int c, stage=0;
	while( EOF != (c=fgetc(zip)) )
	{
			 if( c=='P'              ) stage++;
		else if( c=='K' &&  stage==1 ) stage++;
		else if( c==0x03 && stage==2 ) stage++;
		else if( c==0x04 && stage==3 )
		{
			stage++;
			int x=ftell(zip);
			if( read_header( hdr,buf ) )
				return true;
			fseek( zip,x,SEEK_SET );
		}
		else stage=0;
	}

	return false;
}

////////////////////////////////////////////////////////////////
// Store : �����k�i�[
////////////////////////////////////////////////////////////////

void CZipTool::Unstore( DWORD usz, DWORD csz )
{
	unsigned char* buf = CTool::common_buf;
//--------------------------------------------------------------------//

	int how_much;
	while( csz )
	{
		how_much = csz > XACR_BUFSIZE ? XACR_BUFSIZE : csz;
		if( 0>=(how_much=zipread( buf, how_much )) )
			break;
		zipwrite( buf, how_much );
		csz -= how_much;
	}
}

////////////////////////////////////////////////////////////////
// Deflate �F lzss+huffman�B���݂�pkzip�̃��\�b�h
////////////////////////////////////////////////////////////////

void CZipTool::Inflate( DWORD usz, DWORD csz )
{
	unsigned char* outbuf = CTool::common_buf;
	unsigned char*  inbuf = CTool::common_buf + (XACR_BUFSIZE/2);
//--------------------------------------------------------------------//

	// zlib����
	z_stream_s zs;
	zs.zalloc   = NULL;
	zs.zfree    = NULL;

	// �o�̓o�b�t�@
	int outsiz = (XACR_BUFSIZE/2);
	zs.next_out = outbuf;
	zs.avail_out= outsiz;

	// ���̓o�b�t�@
	int insiz = zipread( inbuf,
		(XACR_BUFSIZE/2) > csz ? csz : (XACR_BUFSIZE/2) );
	if( insiz<=0 )
		return;
	csz        -= insiz;
	zs.next_in  = inbuf;
	zs.avail_in = insiz;

	// �X�^�[�g
	inflateInit2( &zs, -15 );

	// ���ɂ�����͂��I���܂Ń��[�v
	int err = Z_OK;
	while( csz )
	{
		while( zs.avail_out > 0 )
		{
			err = inflate( &zs,Z_PARTIAL_FLUSH );
			if( err!=Z_STREAM_END && err!=Z_OK )
				csz=0;
			if( !csz )
				break;

			if( zs.avail_in<=0 )
			{
				int insiz = zipread( inbuf, (XACR_BUFSIZE/2) > csz ?
												csz : (XACR_BUFSIZE/2) );
				if( insiz<=0 )
				{
					err = Z_STREAM_END;
					csz = 0;
					break;
				}

				csz        -= insiz;
				zs.next_in  = inbuf;
				zs.avail_in = insiz;
			}
		}

		zipwrite( outbuf, outsiz-zs.avail_out );
		zs.next_out  = outbuf;
		zs.avail_out = outsiz;
	}

	// �o�͎c���𖳂����B
	while( err!=Z_STREAM_END )
	{
		err = inflate(&zs,Z_PARTIAL_FLUSH);
		if( err!=Z_STREAM_END && err!=Z_OK )
			break;

		zipwrite( outbuf, outsiz-zs.avail_out );
		zs.next_out  = outbuf;
		zs.avail_out = outsiz;
	}

	// �I��
	inflateEnd(&zs);
}

////////////////////////////////////////////////////////////////
// Shrink �F LZW with partial_clear PkZip 0.x-1.x
////////////////////////////////////////////////////////////////

void CZipTool::Unshrink( DWORD usz, DWORD csz )
{
	// �����T�C�Y = 8192Bytes = 13bits
	unsigned char* stack     = CTool::common_buf;
	unsigned char* suffix_of = CTool::common_buf + 8192 + 1;
	unsigned int*  prefix_of = (unsigned int*)CTool::common_buf + (8192 + 1) * 2;

	#define GetCode() getbits(codesize)
	int left=(signed)usz-1;
	//--------------------------------------------------------------------//


	int codesize, maxcode, free_ent, offset, sizex;
	int code, stackp, finchar, oldcode, incode;

	// �������c
	initbits();
	codesize = 9; // ����bit��
	maxcode  = (1<<codesize) - 1;
	free_ent = 257; // �g���ĂȂ������̈�̍ŏ�
	offset   = 0;
	sizex    = 0;

	// ����������
	for( code=(1<<13); code > 255; code-- )
		prefix_of[code] = -1;
	for( code=255; code >= 0; code-- )
	{
		prefix_of[code] = 0;
		suffix_of[code] = code;
	}

	// ���̑��������
	oldcode = GetCode();
	if( bits_eof )
		return;
	finchar = oldcode;
	BYTE f=finchar;
	zipwrite( &f,1 );

	stackp = 8192;

	while( left>0 && !bits_eof )
	{
		// code ������o��
		code = GetCode();
		if( bits_eof )
			break;

		// clear!
		while( code == 256 )
		{
			switch( GetCode() )
			{
			case 1:
				codesize++;
				if( codesize == 13 )
					maxcode = (1 << codesize);
				else
					maxcode = (1 << codesize) - 1;
				break;

			case 2: // partial_clear !!
				{
					int pr,cd;

					for( cd=257; cd<free_ent; cd++ )
						prefix_of[cd] |= 0x8000;

					for( cd=257; cd<free_ent; cd++)
					{
						pr = prefix_of[cd] & 0x7fff;
						if( pr >= 257 )
							prefix_of[pr] &= 0x7fff;
					}

					for( cd=257; cd<free_ent; cd++ )
						if( (prefix_of[cd] & 0x8000) != 0 )
							prefix_of[cd] = -1;

					cd = 257;
					while( (cd < (1<<13)) && (prefix_of[cd] != -1) )
						cd++;
					free_ent = cd;
				}
				break;
			}

			code = GetCode();
			if( bits_eof )
				break;
		}
		if( bits_eof )
			break;

		// ����ȏꍇ( KwKwK )
		incode = code;
		if( prefix_of[code] == -1 )
		{
            stack[--stackp] = finchar;
			code = oldcode;
		}

		// ��������X�^�b�N��
		while( code >= 257 )
		{
			stack[--stackp] = suffix_of[code];
			code = prefix_of[code];
		}
		finchar = suffix_of[code];
		stack[--stackp] = finchar;
		// �X�^�b�N����o��
		left -= (8192-stackp);
		zipwrite( stack+stackp, (8192-stackp) );
		stackp = 8192;

		// �V�����G���g����ǉ�
		code = free_ent;
		if( code < (1<<13) )
		{
			prefix_of[code] = oldcode;
			suffix_of[code] = finchar;

			do
				code++;
			while( (code < (1<<13)) && (prefix_of[code] != -1) );

			free_ent = code;
		}

		// �O�̂��o���ĂƂ�
		oldcode = incode;
	}

#undef GetCode
}

////////////////////////////////////////////////////////////////
// Reduce �F lz77 + �ςȕ�����(^^; PkZip 0.x
////////////////////////////////////////////////////////////////

void CZipTool::LoadFollowers( BYTE* Slen, BYTE followers[][64] )
{
	for( int x=255; x>=0; x-- )
	{
		Slen[x] = getbits(6);
		for( int i=0; i<Slen[x]; i++ )
			followers[x][i] = getbits(8);
	}
}

void CZipTool::Unreduce( DWORD usz, DWORD csz, int factor )
{
	unsigned char* outbuf = CTool::common_buf;
	unsigned char* outpos=outbuf;
	memset( outbuf,0,0x4000 );
	int left = (signed)usz;
#define RED_FLUSH() zipwrite(outbuf,outpos-outbuf), outpos=outbuf;
#define RED_OUTC(c) {(*outpos++)=c; left--; if(outpos-outbuf==0x4000) RED_FLUSH();}
//--------------------------------------------------------------------//

	static const int Length_table[] = {0, 0x7f, 0x3f, 0x1f, 0x0f};
	static const int D_shift[] = {0, 0x07, 0x06, 0x05, 0x04};
	static const int D_mask[]  = {0, 0x01, 0x03, 0x07, 0x0f};
	static const int B_table[] = {
		8, 1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5,
		5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8};

	initbits();

	BYTE followers[256][64];
	BYTE Slen[256];
	LoadFollowers( Slen, followers );

	int l_stage = 0;
	int l_char  = 0;
	int ch, V, Len;

	while( !bits_eof && left>0 )
	{
		if( Slen[l_char] == 0 )
			ch = getbits(8);
		else
		{
			if( getbits(1) )
				ch = getbits(8);
			else
			{
				int bitsneeded = B_table[ Slen[l_char] ];
				ch = followers[ l_char ][ getbits(bitsneeded) ];
			}
		}

		// Repeater Decode
		switch( l_stage )
		{
		case 0:
			if( ch == 0x90 )// hit�������t���O�Ȃ�A���̃X�e�[�W��
				l_stage++;
			else			// ���̂܂܏o��
				RED_OUTC( ch )
			break;

		case 1:
			if( ch == 0x00 )// hit�t���O�����̂܂܏o��
			{
				RED_OUTC(0x90)
				l_stage = 0;
			}
			else			// �������擾( ���\�b�h�ˑ� )
			{
				V   = ch;
				Len = V & Length_table[factor];
				if( Len == Length_table[factor] )
					l_stage = 2;
				else
					l_stage = 3;
			}
			break;

		case 2:				// ������ǉ��擾
			Len += ch;
			l_stage++;
			break;

		case 3:				// �}�b�`�����������o��( ���\�b�h�ˑ� )
			{
				int i = Len+3;
				int offset = (((V>>D_shift[factor]) & D_mask[factor]) << 8) + ch + 1;
				int op = (((outpos-outbuf)-offset) & 0x3fff);

				while( i-- )
				{
					RED_OUTC( outbuf[op++] );
					op &= 0x3fff;
				}

				l_stage = 0;
			}
			break;
		}
		l_char = ch;
	}

	RED_FLUSH();

#undef RED_FLUSH
#undef RED_OUTC
}

////////////////////////////////////////////////////////////////
// Implode �F lz77 + shanon-fano ? PkZip 1.x
////////////////////////////////////////////////////////////////

void CZipTool::Explode( DWORD usz, DWORD csz, bool _8k, bool littree )
{
	unsigned char* outbuf = CTool::common_buf;
	unsigned char* outpos=outbuf;
	memset( outbuf,0,0x4000 );
	int left = (signed)usz;
#define EXP_FLUSH() zipwrite(outbuf,outpos-outbuf), outpos=outbuf;
#define EXP_OUTC(c) {(*outpos++)=c; left--; if(outpos-outbuf==0x4000) EXP_FLUSH();}
//--------------------------------------------------------------------//

	BYTE ch;
	initbits();

	int dict_bits     = ( _8k ? 7 : 6);
	int min_match_len = (littree ? 3 : 2);
	sf_tree lit_tree; 
	sf_tree length_tree; 
	sf_tree distance_tree; 

	// �؂����[�h
	if( littree ) 
		LoadTree( &lit_tree, 256 );
	LoadTree( &length_tree, 64 ); 
	LoadTree( &distance_tree, 64 ); 

	// �f�[�^����
	while( !bits_eof && left>0 )
	{
		if( getbits(1) ) // ���ʂ̕����f�[�^
		{
			if( littree )
				ch = ReadTree( &lit_tree );
			else
				ch = getbits(8);

			EXP_OUTC( ch );
		}
		else // �X���C�h�����Ƀ}�b�`���Ă�f�[�^
		{
			// ����
			int Distance = getbits(dict_bits);
			Distance |= ( ReadTree(&distance_tree) << dict_bits );

			// ����
			int Length = ReadTree( &length_tree );

			if( Length == 63 )
				Length += getbits(8);
			Length += min_match_len;

			// �o��
			int op = (((outpos-outbuf)-(Distance+1)) & 0x3fff);
			while( Length-- )
			{
				EXP_OUTC( outbuf[op++] );
				op &= 0x3fff;
			}
		}
	}

	EXP_FLUSH();

#undef EXP_OUTC
#undef EXP_FLUSH
}

static void SortLengths( sf_tree* tree )
{ 
	int gap,a,b;
	sf_entry t; 
	bool noswaps;

	gap = tree->entries >> 1; 

	do
	{
		do
		{
			noswaps = true;
			for( int x=0; x<=(tree->entries - 1)-gap; x++ )
			{
				a = tree->entry[x].BitLength;
				b = tree->entry[x + gap].BitLength;
				if( (a>b) || ((a==b) && (tree->entry[x].Value > tree->entry[x + gap].Value)) )
				{
					t = tree->entry[x];
					tree->entry[x] = tree->entry[x + gap];
					tree->entry[x + gap] = t;
					noswaps = false;
				}
			}
		} while (!noswaps);
		
		gap >>= 1;
	} while( gap > 0 );
}

void CZipTool::ReadLengths( sf_tree* tree )
{ 
	int treeBytes,i,num,len;

	treeBytes = getbits(8) + 1;
	i = 0; 

	tree->MaxLength = 0;

	while( treeBytes-- )
	{
		len = getbits(4)+1;
		num = getbits(4)+1;

		while( num-- )
		{
			if( len > tree->MaxLength )
				tree->MaxLength = len;
			tree->entry[i].BitLength = len;
			tree->entry[i].Value = i;
			i++;
		}
	} 
} 

static void GenerateTrees( sf_tree* tree )
{ 
	WORD Code = 0;
	int CodeIncrement = 0, LastBitLength = 0;

	int i = tree->entries - 1;   // either 255 or 63
	while( i >= 0 )
	{
		Code += CodeIncrement;
		if( tree->entry[i].BitLength != LastBitLength )
		{
			LastBitLength = tree->entry[i].BitLength;
			CodeIncrement = 1 << (16 - LastBitLength);
		}

		tree->entry[i].Code = Code;
		i--;
	}
}

static void ReverseBits( sf_tree* tree )
{
	for( int i=0; i<=tree->entries-1; i++ )
	{
		WORD o = tree->entry[i].Code,
			 v = 0,
		  mask = 0x0001,
		  revb = 0x8000;

		for( int b=0; b!=16; b++ )
		{
			if( (o&mask) != 0 )
				v = v | revb;

			revb = (revb >> 1);
			mask = (mask << 1);
		}

		tree->entry[i].Code = v;
	}
}

void CZipTool::LoadTree( sf_tree* tree, int treesize )
{ 
	tree->entries = treesize; 
	ReadLengths( tree ); 
	SortLengths( tree ); 
	GenerateTrees( tree ); 
	ReverseBits( tree ); 
} 

int CZipTool::ReadTree( sf_tree* tree )
{ 
	int bits=0, cur=0, b;
	WORD cv=0;

	while( true )
	{
		b = getbits(1);
		cv = cv | (b << bits);
		bits++;

		while( tree->entry[cur].BitLength < bits )
		{
			cur++;
			if( cur >= tree->entries )
				return -1;
		}

		while( tree->entry[cur].BitLength == bits )
		{
			if( tree->entry[cur].Code == cv )
				return tree->entry[cur].Value;
			cur++;
			if( cur >= tree->entries )
				return -1;
		}
	}
}
