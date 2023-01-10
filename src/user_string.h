//STRING.H
#if !defined(__STRING_H_)
#define __STRING_H_

#ifdef __ENGLISH
//English resource
//S-JIS
#define	IDS_TITLE_DLL					"UNZIP.DLL for WindowsCE\n"
#define	IDS_FMT_FILE_ERROR1				"%s is corrupted or is not supported\n"
#define	IDS_FMT_FILE_ERROR2				"%s could not be extracted"
#define	IDS_FMT_DLG_ERROR1				"Create error"
#define	IDS_FMT_DLG_ERROR2				"Error opening archive"
#define IDS_MSG_SKIP					"Execution was interrupted by user\n"
#define IDS_MSG_PASSWD_NG				"Wrong password"
//UNI-CODE
#define	IDS_TITLE_COMPRESS				_T("unzip.dll Compress")
#define	IDS_TITLE_UNCOMPRESS			_T("unzip.dll Extract")
#define	IDS_MSG_OVERWRITE_FROM			_T("Old :%d-%02d-%02d %2d:%02d:%02d")
#define	IDS_MSG_OVERWRITE_TO			_T("New :%d-%02d-%02d %2d:%02d:%02d")
#else
//Japanese resource
//S-JIS
#define	IDS_TITLE_DLL					"UNZIP.DLL for WindowsCE\n"
#define	IDS_FMT_FILE_ERROR1				"%s �͉��Ă��邩�A�T�|�[�g����Ă��Ȃ��`���ł�\n"
#define	IDS_FMT_FILE_ERROR2				"%s �̉𓀂Ɏ��s���܂���"
#define	IDS_FMT_DLG_ERROR1				"�_�C�A���O�쐬�G���["
#define	IDS_FMT_DLG_ERROR2				"���ɃI�[�v���Ɏ��s���܂���"
#define IDS_MSG_SKIP					"���[�U�[�̗v�]�ŏ��������f����܂���\n"
#define IDS_MSG_PASSWD_NG				"�p�X���[�h����v���܂���"
//UNI-CODE
#define	IDS_TITLE_COMPRESS				_T("unzip.dll ���k")
#define	IDS_TITLE_UNCOMPRESS			_T("unzip.dll ��")
#define	IDS_MSG_OVERWRITE_FROM			_T("���݂̃t�@�C��:%d-%02d-%02d %2d:%02d:%02d")
#define	IDS_MSG_OVERWRITE_TO			_T("�V�����t�@�C��:%d-%02d-%02d %2d:%02d:%02d")
#endif
#endif //__STRING_H_