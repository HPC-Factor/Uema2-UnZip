/////////////////////////////////
// encMain.c
//   ˆ³k‚Ì‚½‚ß‚ÌƒƒCƒ“ƒ‹[ƒ`ƒ“
/////////////////////////////////

#include "decMain.h"

extern int zipMain(int argc, char *argv[]);

int encodeMain(int argc, char *argv[])
{
	int n=0;

//	"Usage : minizip [-o] file.zip [files_to_add]"
	//argv[0] - "unzip.dll"
	//argv[1] - "-a"
	//argv[2] - "test.zip"
	//argv[3] - [base-directory]
	//argv[4] - files-to-add
	n = zipMain(argc, argv);

	return n;
}
