#ifndef AFX_TOOL_H__57AAB1DD_886F_4D25_860C_2AA917B02AC1__INCLUDED_
#define AFX_TOOL_H__57AAB1DD_886F_4D25_860C_2AA917B02AC1__INCLUDED_

//#include "kiutil.h"
//#define XACR_BUFSIZE (0x100000)
// �� must be bigger than 0x100000 !! ( for RAR )
#define XACR_BUFSIZE (0x20000)
// zip �����Ȃ�ŁA200kb ������Ώ\�����ȁ[�Ƃ�

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

// ���΃p�X��^����� [�s����������],[�����K�wmakeDir] ���s��
//static char* pathMake( char* path );
char* pathMake( char* path );

// ��΃p�X��^����� [�����K�wmakeDir]
void pathMakeAbs( char* path );


class CTool
{
public:

////////////////////////////////////////////////
// �h���N���X�쐬�ɂ���MEMO
//
// GetPassword
// �@�p�X���[�h���K�v�ɂȂ����炱����ĂԂ���
// �@�Ăԓx�Ƀ_�C�A���O���o���ă��[�U�ɓ��͂�����
// common_buf
// �@��Ɨp�Ɏ��R�Ɏg���Ă悵�B
// �@XACR_BUFSIZE byte �m�ۂ���Ă���͂��B
// �@
////////////////////////////////////////////////
//
// �R���X�g���N�^
// �@CTool( ���[�`���� )��K���Ăяo������
//
// �f�X�g���N�^
// �@���R��
//
// IsType
// �@�g���q����Ή����ɂ��ǂ�������B�������A�g���q
// �@�ȊO�̃}�g���Ȕ����i�������Ȃ����[�`����
// �@�����ł͕K�� false ��Ԃ��Ȃ��Ă͂Ȃ�Ȃ�
//
// Check
// �@�t�@�C����, �t�@�C���T�C�Y �ƁAcommon_buf �ɓ����Ă���
// �@�t�@�C���擪�� min(XACR_BUFSIZE,fsize) bytes ���g���Č����ɔ���
// �@common_buf �͂��̎����������s�I
//
// Extract
// �@����"fname"���f�B���N�g��"ddir"(=�J�����g)�ɓW�J
//
////////////////////////////////////////////////

// ���z�֐� ////////////////

	virtual bool IsType( const char* ext ){return false;}
	virtual bool Check( const char* fname, unsigned long fsize ) = 0;
	virtual int  Extract( const char* fname, const char* ddir,
		const char **extname=NULL, int extnum=0) = 0;

// �R���X�g���N�^���f�X�g���N�^ ////

protected:
	CTool( const char* name )
		: m_Name(name), m_pNext(NULL), m_pPass(NULL){}
public:
	virtual ~CTool()
		{delete m_pNext;	delete [] m_pPass;}

// ��Ɨp�o�b�t�@ ///////

public:
	static void AllocWorkSpace()
		{common_buf = new unsigned char[XACR_BUFSIZE];}
	static void FreeWorkSpace()
		{delete [] common_buf; common_buf=NULL;}

	static unsigned char* common_buf;

// ���[�`���� ///////////

public:
	const char* GetRoutineName()
		{return m_Name;}
private:
	const char* m_Name;

// �ȈՃ��X�g ///////////

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

// �p�X���[�h ///////////

protected:
	char* GetPassword();
private:
	char* m_pPass;
	static BOOL CALLBACK PassProc( HWND dlg,UINT msg,WPARAM wp,LPARAM lp );

////////////////////////
};

#endif
