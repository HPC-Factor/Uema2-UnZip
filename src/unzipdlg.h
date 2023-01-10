
/* unlhadlg.h */

//HWND hStatic1, hStatic2, hStatic3;
//HWND hProgress;

//BOOL CALLBACK dlgProc(HWND h, UINT m,
BOOL CALLBACK dlgProc(HWND h, UINT m,
						 WPARAM wp, LPARAM lp);
void progressSetRange(int min, int max);
void progressSetStep(int step);
void progressSetPos(int pos);
void progressGetAStep(void);

void dlgSetArchiveName(const char* name);
void dlgSetFileName(const char* name);
void dlgSetFileSize(const int totalSize, const int size);

//int createInquireDialog(const char* a, time_t t, WORD dat, WORD tim);
int createInquireDialog(const char *a, long, WORD, WORD);
BOOL CALLBACK inqDlgProc(HWND h, UINT m,
						 WPARAM wp, LPARAM lp);
