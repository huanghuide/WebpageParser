#include "Common.h"

string gOutputPath = "";
string gBaseName = "";
string gInputHtmlFile = "";
string gHomeUrl = "";

string gTagNames[] = {
    "!doctype",
    "!--",
    "a",
    "abbr",
    "acronym",
    "address",
    "applet",
    "area",
    "article",
    "aside",
    "audio",
    "b",
    "base",
    "basefont",
    "bdi",
    "bdo",
    "big",
    "blockquote",
    "body",
    "br",
    "button",
    "canvas",
    "caption"
    "center",
    "cite",
    "code",
    "col",
    "colgroup",
    "command",
    "datalist",
    "dd",
    "del",
    "details",
    "dir",
    "div",
    "dfn",
    "dialog",
    "dl",
    "dt",
    "em",
    "embed",
    "fieldset",
    "figcaption",
    "figure",
    "font",
    "footer",
    "form",
    "frame",
    "frameset",
    "h1",
    "h2",
    "h3",
    "h4",
    "h5",
    "h6",
    "head",
    "header",
    "hr",
    "html",
    "i",
    "iframe",
    "img",
    "input",
    "ins",
    "isindex",
    "kbd",
    "keygen",
    "label",
    "legend",
    "li",
    "link",
    "map",
    "mark",
    "menu",
    "menuitem",
    "meta",
    "meter",
    "nav",
    "noframes",
    "noscript",
    "object",
    "ol",
    "optgroup",
    "option",
    "output",
    "p",
    "param",
    "pre",
    "progress",
    "q",
    "rp",
    "rt",
    "ruby",
    "s",
    "samp",
    "script",
    "section",
    "select",
    "small",
    "source",
    "span",
    "strike",
    "strong",
    "style",
    "sub",
    "summary",
    "sup",
    "table",
    "tbody",
    "td",
    "textarea",
    "tfoot",
    "th",
    "thead",
    "time",
    "title",
    "tr",
    "track",
    "tt",
    "u",
    "ul",
    "var",
    "video",
    "wbr",
    "xmp",
    "g:plusone",
    "t-button",
    "template"
};

vector<string> gStartTagNames;
vector<string> gEndTagNames;

int GetStringHashKey(string str)
{
    return GetStringHashKey((char*)(str.c_str()), str.length());
}

int GetStringHashKey(char* pData, int dataLen)
{
    int seed = 131;
    int hash = 0;

    for (int index = 0; index < dataLen; index++)
    {
        hash = hash * seed + *(pData + index);
    }

    return hash;
}

void InitTagNames()
{
    int index = 0;

    for (index = 0; index < sizeof(gTagNames) / sizeof(string); index++)
    {
        string name1 = "<" + gTagNames[index] + " ";
        string name2 = "<" + gTagNames[index] + ">";

        gStartTagNames.push_back(name1);
        gStartTagNames.push_back(name2);
    }

    // Special start tag name
    gStartTagNames.push_back("<!--");
    gStartTagNames.push_back("<br/>");

    for (index = 2; index < sizeof(gTagNames) / sizeof(string); index++)
    {
        string name = "</" + gTagNames[index] + ">";

        gEndTagNames.push_back(name);
    }

    // Special end tag name
    gEndTagNames.push_back("-->");
}

string trim(string s, string ss)
{
    const string drop = ss;
    // trim right
    s.erase(s.find_last_not_of(drop)+1);
    // trim left
    return s.erase(0,s.find_first_not_of(drop));
}

string ltrim(string s)
{
    const string drop = " ";
    // trim left
    return s.erase(0,s.find_first_not_of(drop));
}

string rtrim(string s)
{
    const string drop = " ";
    // trim right
    return s.erase(s.find_last_not_of(drop)+1);
}

void StringToUpper(char* pStr, int strLen)
{
    for (int index = 0; index < strLen; index++)
    {
        pStr[index] = toupper(pStr[index]);
    }
}

void StringToLower(char* pStr, int strLen)
{
    for (int index = 0; index < strLen; index++)
    {
        pStr[index] = tolower(pStr[index]);
    }
}

int ConvertHexStrToInt(string hexStr)
{
    int ret = 0;

    for (int index = 0; index < hexStr.length(); index++)
    {
        if ((hexStr.at(index) >= '0') && (hexStr.at(index) <= '9'))
        {
            ret = ret * 16 + (hexStr.at(index) - '0');
        }
        else if ((hexStr.at(index) >= 'a') && (hexStr.at(index) <= 'f'))
        {
            ret = ret * 16 + (hexStr.at(index) - 'a' + 10);
        }
        else if ((hexStr.at(index) >= 'A') && (hexStr.at(index) <= 'F'))
        {
            ret = ret * 16 + (hexStr.at(index) - 'A' + 10);
        }
    }

    return ret;
}

void CheckAndCreateFolder(string folderPath)
{
    if (access(folderPath.c_str(), F_OK) != 0)
    {
        if (mkdir(folderPath.c_str(), 0777) != 0)
        {
            printf("Failed to create folder %s\n", folderPath.c_str());
        }
    }
}

string GetHomeUrl(string url)
{
    string homeUrl = "";
    string tmpUrl = trim(url, " ");

    if ((tmpUrl.length() >= 4) && ((tmpUrl.substr(0, 4) == "http") || (tmpUrl.substr(0, 4) == "HTTP")))
    {
        int doubleSlashPos = tmpUrl.find("//");
        if (doubleSlashPos != string::npos)
        {
            int slashPos = tmpUrl.find("/", doubleSlashPos + 2);
            if (slashPos != string::npos)
            {
                homeUrl = tmpUrl.substr(0, slashPos);
            }
            else
            {
                homeUrl = tmpUrl;
            }
        }
    }

    return homeUrl;
}

string TransCodeIntoUtf8WithIconv(string inputStr, string inputCharset)
{
    string retStr = "";
    string tmpCharset = inputCharset;

    transform(tmpCharset.begin(), tmpCharset.end(), tmpCharset.begin(), ::toupper);
    iconv_t icv = iconv_open("UTF-8", tmpCharset.c_str());

    if (icv != NULL)
    {
        int argument = 1;
        if (iconvctl(icv, ICONV_SET_DISCARD_ILSEQ, &argument) != 0)
        {
            printf("Cannot enable iconv illegal sequence discard and continue feature!\n");

            iconv_close(icv);
        }
        else
        {
            string inBuf = inputStr;
            string outBuf = "";

            outBuf.resize(inputStr.length() * 2);

            char* pInBuf = const_cast<char*>(inBuf.c_str());
            char* pOutBuf = const_cast<char*>(outBuf.c_str());

            size_t inLen = inputStr.length();
            size_t outLen = inputStr.length() * 2;

            try
            {
                iconv(icv, &pInBuf, &inLen, &pOutBuf, &outLen);
            }
            catch (exception& e)
            {
            }

            retStr = outBuf;

            iconv_close(icv);
        }
    }
    else
    {
        printf("cannot open iconv!\n");
    }

    return retStr;
}

void StringReplace(string& strBig, const string& strsrc, const string& strdst)
{
    string::size_type pos=0;
    string::size_type srclen=strsrc.size();
    string::size_type dstlen=strdst.size();

    while( (pos=strBig.find(strsrc, pos)) != string::npos)
    {
        strBig.replace(pos, srclen, strdst);
        pos += dstlen;
    }
}
