#ifndef _COMMON_H_
#define _COMMON_H_

#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <exception>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <iconv.h>
#include <signal.h>
#include <time.h>
#include <iconv.h>
#include <locale.h>

#include <zlib.h>
#include <zconf.h>

using namespace std;

#define BUF_LEN     (64 * 1024)
#define LINE_LEN    (1 * 1024)
#define MSG_LEN     (2 * 1024)

struct WebSiteConfigInfo
{
    string startUrl;
    string abbrName;
    string searchPrefix;
    int maxFetchTime;
};

extern string gTagNames[];
extern vector<string> gStartTagNames;
extern vector<string> gEndTagNames;

extern string gOutputPath;
extern string gBaseName;
extern string gInputHtmlFile;
extern string gHomeUrl;

void InitTagNames();

string trim(string s, string ss = " ");
//string trim(string s);
string ltrim(string s);
string rtrim(string s);

int GetStringHashKey(string str);
int GetStringHashKey(char* pData, int dataLen);

void StringToUpper(char* pStr, int strLen);
void StringToLower(char* pStr, int strLen);

int ConvertHexStrToInt(string hexStr);

void CheckAndCreateFolder(string folderPath);

string GetHomeUrl(string url);

string TransCodeIntoUtf8WithIconv(string inputStr, string inputCharset);

void StringReplace(string& strBig, const string& strsrc, const string& strdst);

#endif




