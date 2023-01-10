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
#define	IDS_FMT_FILE_ERROR1				"%s は壊れているか、サポートされていない形式です\n"
#define	IDS_FMT_FILE_ERROR2				"%s の解凍に失敗しました"
#define	IDS_FMT_DLG_ERROR1				"ダイアログ作成エラー"
#define	IDS_FMT_DLG_ERROR2				"書庫オープンに失敗しました"
#define IDS_MSG_SKIP					"ユーザーの要望で処理が中断されました\n"
#define IDS_MSG_PASSWD_NG				"パスワードが一致しません"
//UNI-CODE
#define	IDS_TITLE_COMPRESS				_T("unzip.dll 圧縮")
#define	IDS_TITLE_UNCOMPRESS			_T("unzip.dll 解凍")
#define	IDS_MSG_OVERWRITE_FROM			_T("現在のファイル:%d-%02d-%02d %2d:%02d:%02d")
#define	IDS_MSG_OVERWRITE_TO			_T("新しいファイル:%d-%02d-%02d %2d:%02d:%02d")
#endif
#endif //__STRING_H_