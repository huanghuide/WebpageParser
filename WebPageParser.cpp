#include "WebPageParser.h"

WebPageParser::WebPageParser(std::string homeUrl, std::string filePath) :
    mHomeUrl(homeUrl),
    mWebPage(""),
    mStartNode(NULL),
    mCurrentNode(NULL)
{
    ReadWebPageFromFile(filePath);

    BuildTree();
    
    //mCharset = GetCharsetFromTree();
    mCharset = "UTF-8";

    mTitle = GetTitleFromTree();
    FoundAllUrls();
    
    //FoundContent();    
}

WebPageParser::~WebPageParser()
{
    RecurseRemoveNode(mStartNode);
}

void WebPageParser::ReadWebPageFromFile(string filePath)
{
    char* pRead;
    char lineBuf[LINE_LEN];

    FILE* fp = fopen(filePath.c_str(), "r"); 

    if (fp != NULL)
    {
        memset(lineBuf, 0, LINE_LEN);
        pRead = fgets(lineBuf, LINE_LEN, fp);

        while (pRead != NULL)
        {
            int length = strlen(lineBuf);

            if (lineBuf[length - 1] == '\n')
            {
                lineBuf[length - 1] = '\0'; // Remove '\n'
            }

            if (strlen(lineBuf) > 0)
            {
                mWebPage.append(lineBuf);
            }

            memset(lineBuf, 0, LINE_LEN);
            pRead = fgets(lineBuf, LINE_LEN, fp);
        }

        fclose(fp);
    }
}

void WebPageParser::BuildTree()
{
    int curPos = 0;
    int lastPos = 0;

    while ((curPos != string::npos) && (curPos < mWebPage.length()))
    {
        NodeType curType = JudgeStatus(curPos);
        bool tmpRet = false;

        //printf("[WebPageParser::BuildTree] curPos %d, curType %d, string is %s\n", curPos, curType, mWebPage.substr(curPos, 100).c_str());

        if (curType == TAG_START)
        {
            tmpRet = ProcessTagStart(curPos);
        }
        else if (curType == TAG_END)
        {
            tmpRet = ProcessTagEnd(curPos);
        }
        else if (curType == CONTENT)
        {
            tmpRet = ProcessContent(curPos);
        }

        if (!tmpRet)
        {
            printf("Failed to process the string! curPos %d\n", curPos);

            int leftLen = mWebPage.length() - curPos;
            string leftStr = (leftLen > 200) ? mWebPage.substr(curPos, 200) : mWebPage.substr(curPos);

            printf("The rest string is %s\n", leftStr.c_str());

            break;
        }

        if (curPos > lastPos)
        {
            lastPos = curPos;
        }
        else
        {
            printf("None is processed in the loop, break to avoid dead loop! curPos %d\n", curPos);

            int leftLen = mWebPage.length() - curPos;
            string leftStr = (leftLen > 200) ? mWebPage.substr(curPos, 200) : mWebPage.substr(curPos);

            printf("The rest string is %s\n", leftStr.c_str());

            break;
        }
    }

    //printf("Finish buiding the tree. curPos %d, mWebPage length %d\n", curPos, mWebPage.length());
}

WebPageParser::NodeType WebPageParser::JudgeStatus(int curPos)
{
    WebPageParser::NodeType ret = CONTENT;

    if (mWebPage.at(curPos) == '<')
    {
        int restLen = mWebPage.length() - curPos;
        string str = (restLen > 30) ? mWebPage.substr(curPos, 30) : mWebPage.substr(curPos);

        if (IsTagStart(str))
        {
            ret = TAG_START;
        }
        else if (IsTagEnd(str))
        {
            ret = TAG_END;
        }
        else
        {
            ret = CONTENT;
        }
    }
    else
    {
        ret = CONTENT;
    }

    return ret;
}

bool WebPageParser::IsTagStart(string str)
{
    bool ret = false;

    string tmpStr = str;
    transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

    for (int index = 0; index < gStartTagNames.size(); index++)
    {
        int pos = tmpStr.find(gStartTagNames[index]);

        if (pos == 0)
        {
            ret = true;
            break;
        } 
    }

    return ret;
}

bool WebPageParser::IsTagEnd(string str)
{
    bool ret = false;

    string tmpStr = str;
    transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(), ::tolower);

    for (int index = 0; index < gEndTagNames.size(); index++)
    {
        int pos = tmpStr.find(gEndTagNames[index]);

        if (pos == 0)
        {
            ret = true;
            break;
        } 
    }

    return ret;
}

bool WebPageParser::ProcessTagStart(int& curPos)
{    
    bool ret = false;

    // 1. Process the comment case: <!-- ******* -->
    if ((mWebPage.length() - curPos) > 4)
    {
        string tmpStr = mWebPage.substr(curPos, 4);
        if (tmpStr == "<!--")
        {
            Node* pNewNode = InitNewNode(COMMENTS);
            PutNewNodeIntoTree(pNewNode);

            pNewNode->tag_name = "!--";

            int pos = mWebPage.find("-->", curPos);
            if (pos != string::npos)
            {
                pNewNode->content = trim(mWebPage.substr(curPos + 4, (pos - curPos - 4)), " ");
                pNewNode->meetEnd = true;

                curPos = pos + 3;
                ret = true;
            }

            return ret;
        }
    }

    // 2. normal tag cases
    int rightBracketPos = mWebPage.find('>', (curPos + 1));

    // Handle the case that <tag attr="<a>bcd</a>">
    int quotationPos = mWebPage.find('\"', (curPos + 1));
    while ((quotationPos != string::npos) && (rightBracketPos != string::npos) && (quotationPos < rightBracketPos))
    {
        quotationPos = mWebPage.find('\"', (quotationPos + 1));
        rightBracketPos = mWebPage.find('>', (quotationPos + 1));

        quotationPos = mWebPage.find('\"', (quotationPos + 1));
    }

    // Handle the case that <tag attr='<a>bcd</a>'>
    int singleQuotationPos = mWebPage.find('\'', (curPos + 1));
    while ((singleQuotationPos != string::npos) && (rightBracketPos != string::npos) && (singleQuotationPos < rightBracketPos))
    {
        singleQuotationPos = mWebPage.find('\'', (singleQuotationPos + 1));
        rightBracketPos = mWebPage.find('>', (singleQuotationPos + 1));

        singleQuotationPos = mWebPage.find('\'', (singleQuotationPos + 1));
    }

    if (rightBracketPos != string::npos)
    {
        string tagStr = mWebPage.substr((curPos + 1), (rightBracketPos - curPos - 1));
        tagStr = trim(tagStr, " ");
        //printf("ProcessTagStart: tagStr is %s\n", tagStr.c_str());

        if (!tagStr.empty())
        {
            bool isTagNameFound = false;
            int pos = 0;

            Node* pNewNode = InitNewNode(TAG_START);
            PutNewNodeIntoTree(pNewNode);

            while (pos < tagStr.length())
            {
                int nextBlank = tagStr.find(' ', pos);

                if (nextBlank != string::npos)
                {
                    ParseTagBlock(pNewNode, !isTagNameFound, tagStr.substr(pos, nextBlank - pos));
                    pos = nextBlank + 1;
                }
                else
                {
                    int specialEndPos = tagStr.find("\"/", pos);

                    if (specialEndPos != string::npos)
                    {
                        ParseTagBlock(pNewNode, !isTagNameFound, tagStr.substr(pos, specialEndPos - pos));
                        pos = specialEndPos + 1;
                    }
                    else
                    {
                        ParseTagBlock(pNewNode, !isTagNameFound, tagStr.substr(pos));
                        pos = tagStr.length();
                    }
                }

                if (!isTagNameFound)
                {
                    isTagNameFound = true;
                }
            }

            curPos = rightBracketPos + 1; 
            CheckTagSpecialEnd(pNewNode, curPos);

            ret = true;
        }   
        else
        {
            curPos = rightBracketPos + 1;
            ret = true;
        }
    }

    return ret;
}

void WebPageParser::ParseTagBlock(Node* pNode, bool isTagName, string block)
{
    if (isTagName)
    {
        string tagName = block;
        transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

        pNode->tag_name = tagName;
    }
    else
    {
        int pos = block.find("=");
        
        if (pos != string::npos)
        {
            string name = block.substr(0, pos);
            string value = (pos == (block.length() - 1)) ? "" : block.substr(pos + 1);

            name = trim(name, " ");

            value = trim(value, " ");
            value = trim(value, "\"");

            if (!name.empty())
            {
                pNode->tag_attrs[name] = value;
            }
        }
        else
        {
            string trimBlock = trim(block, " ");

            pNode->tag_attrs[trimBlock] = "";
        }
    }
}

WebPageParser::Node* WebPageParser::InitNewNode(NodeType type)
{
    Node* newNode = new Node;

    newNode->type = type;
    newNode->meetEnd = false;
    newNode->tag_name = "";
    newNode->tag_attrs.clear();
    newNode->content = "";
    newNode->upperNode = NULL;
    newNode->leftNode = NULL;
    newNode->rightNode = NULL;

    return newNode;
}

void WebPageParser::PutNewNodeIntoTree(Node* newNode)
{
    if (mStartNode == NULL)
    {
        mStartNode = newNode;
    }
    else
    {
        if (mCurrentNode->meetEnd)
        {
            if (mCurrentNode->leftNode == NULL)
            {
                mCurrentNode->leftNode = newNode;
            }
            else
            {
                printf("Node Tree Error! Left node is not NULL!\n");
            }
        }
        else
        {
            if (mCurrentNode->rightNode == NULL)
            {
                mCurrentNode->rightNode = newNode;
            }
            else
            {
                printf("Node Tree Error! Right node is not NULL!\n");
            }
        }

        newNode->upperNode = mCurrentNode;
    }

    mCurrentNode = newNode;
}

bool WebPageParser::CheckTagSpecialEnd(Node* pNode, int& curPos)
{
    bool ret = false;

    string tagName = pNode->tag_name;

    if ( (tagName.at(0) == '!') || // case: <!DOCTYPE html>
        (tagName == "link") || // case: <link rel="stylesheet" type="text/css" href="http://tech.sina.com.cn/css/717/20140827/index2014/top.css">
        (tagName == "meta")) // case: <meta name="stencil" content="PGLS000423">
    {
        pNode->meetEnd = true;
        ret = true;
    }
    else
    {
        map<string, string>::iterator iter = pNode->tag_attrs.begin();

        while (iter != pNode->tag_attrs.end())
        {
            if (iter->first == "/")
            {
                pNode->meetEnd = true;
                break;
            }

            iter++;
        }

        ret = true;
    }

    // special processing for <script>
    if ((!pNode->meetEnd) && (tagName == "script"))
    {
        ProcessTagScript(pNode, curPos);
    }

    return ret;
}

void WebPageParser::ProcessTagScript(Node* pNode, int& curPos)
{
    int endPos1 = mWebPage.find("</script>", curPos);
    int endPos2 = mWebPage.find("</SCRIPT>", curPos);

    int endPos = string::npos;

    if ((endPos1 != string::npos) && (endPos2 != string::npos))
    {
        endPos = endPos1 < endPos2 ? endPos1 : endPos2;
    }
    else if ((endPos1 == string::npos) && (endPos2 != string::npos))
    {
        endPos = endPos2;
    }
    else if ((endPos1 != string::npos) && (endPos2 == string::npos))
    {
        endPos = endPos1;
    }
    else
    {
        endPos = string::npos;
    }

    if (endPos != string::npos)
    {
        pNode->content = mWebPage.substr(curPos, (endPos - curPos));
        pNode->meetEnd = true;

        curPos = endPos + string("</script>").length();
    }
}

bool WebPageParser::ProcessTagEnd(int& curPos)
{
    bool ret = false;

    int rightBracketPos = mWebPage.find('>', (curPos + 1));

    if ((rightBracketPos != string::npos) && (rightBracketPos - curPos >= 3))
    {
        string tagName = mWebPage.substr(curPos + 2, (rightBracketPos - curPos - 2));
        transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

        //printf("ProcessTagEnd: tagName is %s\n", tagName.c_str());

        if (mCurrentNode != NULL)
        {
            if (!(mCurrentNode->meetEnd))
            {
                if (mCurrentNode->tag_name == tagName)
                {
                    mCurrentNode->meetEnd = true;
                    curPos = rightBracketPos + 1;

                    ret = true;
                }
                else
                {
                    mCurrentNode->meetEnd = true;

                    ret = CheckAndSetUpperNodeEnd(mCurrentNode, tagName);

                    if (ret)
                    {
                        curPos = rightBracketPos + 1;
                    }
                }
            }
            else
            {
                ret = CheckAndSetUpperNodeEnd(mCurrentNode, tagName);

                if (ret)
                {
                    curPos = rightBracketPos + 1;
                }
            }
        }
    }

    return ret;
}

WebPageParser::Node* WebPageParser::FindUpperNotEndNode(Node* curNode)
{
    Node* ret = NULL;
    Node* node = curNode;

    while (node != NULL)
    {
        if (node->meetEnd)
        {
            node = node->upperNode;
        }
        else
        {
            ret = node;
            break;
        }
    }

    return ret;
}

bool WebPageParser::CheckAndSetUpperNodeEnd(Node* curNode, string tagName)
{
    bool ret = false;

    Node* pNode = FindUpperNotEndNode(curNode);
    while (pNode != NULL)
    {
        if (pNode->tag_name == tagName)
        {
            if (!(pNode->meetEnd))
            {
                pNode->meetEnd = true;
                mCurrentNode = pNode;

                ret = true;
            }
            else
            {
                printf("The upper tag name has been set to end! tagName %s\n", tagName.c_str());
            }

            break;
        }
        else
        {
            pNode->meetEnd = true;
        }    

        pNode = FindUpperNotEndNode(pNode);               
    }

    return ret;   
}

bool WebPageParser::ProcessContent(int& curPos)
{
    bool ret = false;

    int pos = curPos + 1;
    bool foundFlag = false;

    while (pos < mWebPage.length())
    {
        pos = mWebPage.find("<", pos);

        if (pos != string::npos)
        {
            NodeType type = JudgeStatus(pos);

            if ((type == TAG_START) || (type == TAG_END))
            {
                string content = mWebPage.substr(curPos, (pos - curPos));

                RemoveHtmlCharacterEntities(content);
                TrimContent(content);

                if (!content.empty())
                {
                    Node* pNewNode = InitNewNode(CONTENT);
                    PutNewNodeIntoTree(pNewNode);

                    pNewNode->meetEnd = true;
                    pNewNode->content = content;
                }

                curPos = pos;
                foundFlag = true;
                ret = true;
                break;
            }
            else
            {
                pos = pos + 1;
            }
        }
    }

    if ((!foundFlag) && (pos >= mWebPage.length())) // Content is the end
    {
        string content = mWebPage.substr(curPos);

        RemoveHtmlCharacterEntities(content);
        TrimContent(content);

        if (!content.empty())
        {
            Node* pNewNode = InitNewNode(CONTENT);
            PutNewNodeIntoTree(pNewNode);

            pNewNode->meetEnd = true;
            pNewNode->content = content;
        }

        curPos = mWebPage.length();
        foundFlag = true;

        ret = true;        
    }

    return ret;
}

void WebPageParser::TrimContent(string& content)
{
    while (!content.empty())
    {
        if ((content.at(0) == ' ') ||
            (content.at(0) == '\t') ||
            (content.at(0) == '\r') ||
            (content.at(0) == '\n'))
        {
            content.erase(content.begin());
        }
        else
        {
            break;
        }
    }

    while (!content.empty())
    {
        if ((content.at(content.length() - 1) == ' ') ||
            (content.at(content.length() - 1) == '\t') ||
            (content.at(content.length() - 1) == '\r') ||
            (content.at(content.length() - 1) == '\n'))
        {
            string::iterator iter = content.end();
            iter--;
            content.erase(iter);
        }
        else
        {
            break;
        }
    }
}

void WebPageParser::FoundAllUrls()
{
    RecurseSearchUrl(mStartNode);


    set<string>::iterator iter = mFoundUrls.begin();
    while (iter != mFoundUrls.end())
    {
        //printf("Url: %s\n", (*iter).c_str());
        iter++;
    }

    //printf("\n");

}

void WebPageParser::RecurseSearchUrl(Node* pNode)
{
    if (pNode == NULL)
    {
        return;
    }
    else
    {
        map<string, string>::iterator iter = pNode->tag_attrs.begin();
        while (iter != pNode->tag_attrs.end())
        {
            if ((iter->first == "href") || (iter->first == "src"))
            {
                string findUrl = iter->second;
                trim(findUrl, " ");

                if ((findUrl.length() >= 4) && 
                    ((findUrl.substr(0, 4) == "http") || (findUrl.substr(0, 4) == "HTTP")))
                {
                    if (mFoundUrls.find(findUrl) == mFoundUrls.end())
                    {
                        mFoundUrls.insert(findUrl);
                    }
                }
                else if ((findUrl.length() >= 1) && (findUrl.at(0) == '/'))
                {
                    string compositeUrl = "";

                    if ((findUrl.length() >= 2) && (findUrl.at(1) == '/'))
                    {
                        compositeUrl = "http:" + findUrl;
                    }
                    else
                    {
                        compositeUrl = mHomeUrl + findUrl;
                    }
                    

                    if (mFoundUrls.find(compositeUrl) == mFoundUrls.end())
                    {
                        mFoundUrls.insert(compositeUrl);
                    }
                }
                else if (findUrl.length() >= 4)
                {
                    string compositeUrl = "";
                    size_t dotPos = findUrl.find(".");
                    size_t slashPos = findUrl.find("/");

                    if ((dotPos != string::npos) && (slashPos != string::npos) && (dotPos < slashPos))
                    {
                        compositeUrl = "http://" + findUrl;
                    }
                    else
                    {
                        compositeUrl = mHomeUrl + "/" + findUrl;
                    }
                    
                    
                    if (mFoundUrls.find(compositeUrl) == mFoundUrls.end())
                    {
                        mFoundUrls.insert(compositeUrl);
                    }
                }
            }

            iter++;
        }
    }

    RecurseSearchUrl(pNode->rightNode);
    RecurseSearchUrl(pNode->leftNode);
}

string WebPageParser::FoundContent(bool levelFlag)
{
    Node* pDivWithMaxP = NULL;
    int tagCounter = -100000;
    string retContent = "";

    //RecurseGetDivWithMaxP(mStartNode, pDivWithMaxP, tagCounter);
    //RecurseGetContent(pDivWithMaxP, retContent);

    RecurseGetContent(mStartNode, retContent, levelFlag);

    if (mCharset != "UTF-8")
    {
        //printf("Need to transit code from %s to UTF-8!\n", mCharset.c_str());
        //retContent = TransCodeIntoUtf8WithIconv(retContent, mCharset);
    }

//    printf("The pure content is:\n");
//    printf("%s\n", retContent.c_str());

    return retContent;
}

void WebPageParser::RemoveHtmlCharacterEntities(string& content)
{
    StringReplace(content, "&nbsp;", "");
    StringReplace(content, "&lt;", "<");
    StringReplace(content, "&gt;", ">");
    StringReplace(content, "&amp;", "&");
    StringReplace(content, "&quot;", "\"");
    StringReplace(content, "&apos;", "\'");
    StringReplace(content, "&cent;", "");
    StringReplace(content, "&pound;", "");
    StringReplace(content, "&yen;", "");
    StringReplace(content, "&euro;", "");
    StringReplace(content, "&copy;", "");
    StringReplace(content, "&reg;", "");
    StringReplace(content, "&bull;", "");
    StringReplace(content, "&raquo;", ">>");
    StringReplace(content, "&laquo;", "<<");
    StringReplace(content, "&rsaquo;", ">");
    StringReplace(content, "&lsaquo;", "<");
    StringReplace(content, "&rdquo;", "\'\'");
    StringReplace(content, "&ldquo;", "\'\'");
    StringReplace(content, "&rsquo;", "\'");
    StringReplace(content, "&lsquo;", "\'");
}

void WebPageParser::RecurseGetDivWithMaxP(Node* pNode, Node*& pDivWithMaxP, int& tagCounter)
{
    if ((pNode != NULL) && (pNode->type == TAG_START) && (pNode->tag_name == "div"))
    {
        bool isTagPBelowNodeFlag = IsTagPBelowNode(pNode->rightNode);
        if (isTagPBelowNodeFlag)
        {
            int localTagPCounter = 0;
            int localTagACounter = 0;
            int localContentLen = 0;

            RecurseCheckDiv(pNode->rightNode, localTagPCounter, localTagACounter, localContentLen);            
            int localTagCounter = localTagPCounter * 100 - localTagACounter * 50 + localContentLen;

            //printf("isTagPBelowNodeFlag %d, localTagCounter %d, localTagPCounter %d, localTagACounter %d, localContentLen %d\n", 
                //isTagPBelowNodeFlag, localTagCounter, localTagPCounter, localTagACounter, localContentLen);

            if (localTagCounter > tagCounter)
            {
                pDivWithMaxP = pNode->rightNode;
                tagCounter = localTagCounter;
            }
        }
    }

    if (pNode == NULL)
    {
        return;
    }

    RecurseGetDivWithMaxP(pNode->rightNode, pDivWithMaxP, tagCounter);
    RecurseGetDivWithMaxP(pNode->leftNode, pDivWithMaxP, tagCounter);
}

void WebPageParser::RecurseCheckDiv(Node* pNode, int& tagPCounter, int& tagACounter, int& contentLen)
{
    if (pNode == NULL)
    {
        return;
    }
    else
    {
        if (pNode->type == TAG_START)
        {
            if ((pNode->tag_name == "p") || (pNode->tag_name == "span") || (pNode->tag_name == "h1")  || (pNode->tag_name == "h2") ||
                (pNode->tag_name == "h3") || (pNode->tag_name == "h4") || (pNode->tag_name == "h5")  || (pNode->tag_name == "h6"))
            {
                tagPCounter++;
            }

            if (pNode->tag_name == "a")
            {
                tagACounter++;
            }
        }
        else if ((pNode->type == CONTENT) && (pNode->upperNode != NULL) && !((pNode->upperNode->type == TAG_START) && (pNode->upperNode->tag_name == "style")))
        {
            contentLen += pNode->content.length();
        }
    }

    RecurseCheckDiv(pNode->rightNode, tagPCounter, tagACounter, contentLen);
    RecurseCheckDiv(pNode->leftNode, tagPCounter, tagACounter, contentLen);
}

bool WebPageParser::IsTagPBelowNode(Node* pNode)
{
    bool ret = false;

    Node* pTmpNode = pNode;
    while (pTmpNode != NULL)
    {
        if ((pTmpNode->type == TAG_START) && 
            ((pTmpNode->tag_name == "p") || (pTmpNode->tag_name == "span") || (pTmpNode->tag_name == "h1")  || (pTmpNode->tag_name == "h2") ||
            (pTmpNode->tag_name == "h3") || (pTmpNode->tag_name == "h4") || (pTmpNode->tag_name == "h5")  || (pTmpNode->tag_name == "h6")))
        {
            ret = true;
            break;
        }

        pTmpNode = pTmpNode->leftNode;
    }

    return ret;
}

/*
int WebPageParser::GetNodeLevel(Node* pNode)
{
    int ret = 0;
    Node* pTmp = pNode;

    while (pTmp != NULL)
    {
        if (pTmp->upperNode != NULL)
        {
            if (pTmp->upperNode->rightNode == pTmp)
            {
                ret++;
            }
        }

        pTmp = pTmp->upperNode;
    }

    return ret;
}
*/

string WebPageParser::GetNodeLevel(Node* pNode)
{
    vector<std::string> buf;
    string ret = "";

    Node* pTmp = pNode;

    while (pTmp != NULL)
    {
        if (pTmp->upperNode != NULL)
        {
            if (pTmp->upperNode->rightNode == pTmp)
            {
                buf.push_back("R");
            }
            else if (pTmp->upperNode->leftNode == pTmp)
            {
                buf.push_back("L");
            }
        }
        else
        {
            // The root is the left node of NULL
            buf.push_back("L");
        }

        pTmp = pTmp->upperNode;
    }

    int len = buf.size();
    char tmp[128];

    for (int index = len - 1; index >= 0; index--)
    {
        if (buf[index] == "L")
        {
            sprintf(tmp, "L%d", (len - 1 - index));
        }
        else
        {
            sprintf(tmp, "R%d", (len - 1 - index));
        }

        ret += tmp;

        if (index != 0)
        {
            ret += "-";
        }
    }

    return ret;
}

void WebPageParser::RecurseGetContent(Node* pNode, string& cumStr, bool levelFlag)
{
    if ((pNode != NULL) && 
        ((pNode->type == CONTENT) || ((pNode->type == TAG_START) && (pNode->tag_name == "img"))) && 
        (pNode->upperNode != NULL) && 
        !((pNode->upperNode->type == TAG_START) && (pNode->upperNode->tag_name == "style")))
    {
        //printf("%s\n", pNode->content.c_str());
        if (levelFlag)
        {
            /*
            int level = GetNodeLevel(pNode);
            char buf[50];
            sprintf(buf, "NodeLevel(%d)|", level);
            */

            string buf = GetNodeLevel(pNode);
            buf += "|";

            if (pNode->type == CONTENT)
            {
                cumStr += (buf + pNode->content + "\n");
            }
            else if ((pNode->type == TAG_START) && (pNode->tag_name == "img"))
            {
                cumStr += (buf + "img: ");

                map<string, string>::iterator iter = pNode->tag_attrs.begin();
                while (iter != pNode->tag_attrs.end())
                {
                    cumStr += "[" + iter->first + "]=" + iter->second + "; ";
                    iter++;
                }

                cumStr += "\n";
            }
        }
        else
        {
            cumStr += (pNode->content + "\n");
        }
    }

    if (pNode == NULL)
    {
        return;
    }

    RecurseGetContent(pNode->rightNode, cumStr, levelFlag);
    RecurseGetContent(pNode->leftNode, cumStr, levelFlag);
}

string WebPageParser::GetCharsetFromTree()
{
    string charsetStr = "";

    RecurseGetCharset(mStartNode, charsetStr);

    transform(charsetStr.begin(), charsetStr.end(), charsetStr.begin(), ::toupper);

    //printf("The charset is %s\n\n", charsetStr.c_str());

    return charsetStr;
}

void WebPageParser::RecurseGetCharset(Node* pNode, string& charsetStr)
{
    if ((pNode != NULL) && (pNode->type == TAG_START) && (pNode->tag_name == "meta"))
    {
        // CASE 1: charset is the key, value is the key value
        map<string, string>::iterator iter = pNode->tag_attrs.begin();
        while (iter != pNode->tag_attrs.end())
        {
            if ((iter->first == "charset") || (iter->first == "CHARSET"))
            {
                charsetStr = iter->second;

                break;
            }

            iter++;
        }

        // CASE 2: charset and value is in the "content" value
        if (charsetStr.empty())
        {
            map<string, string>::iterator iter = pNode->tag_attrs.begin();
            while (iter != pNode->tag_attrs.end())
            {
                string value = iter->second;

                int pos1 = value.find("charset");
                int pos2 = value.find("CHARSET");
                int equalPos = string::npos;
                int endPos = string::npos;

                if (pos1 != string::npos)
                {
                    equalPos = value.find("=", (pos1 + 7));
                }
                else if (pos2 != string::npos)
                {
                    equalPos = value.find("=", (pos1 + 7));
                }

                if (equalPos != string::npos)
                {
                    endPos = equalPos + 1;

                    while (endPos < value.length())
                    {
                        if ((value.at(endPos) == ' ') ||
                            (value.at(endPos) == ',') ||
                            (value.at(endPos) == '\"') ||
                            (value.at(endPos) == ';') ||
                            (value.at(endPos) == '>'))
                        {
                            break;
                        }

                        endPos++;
                    }
                }

                if ((equalPos != string::npos) && (endPos != string::npos))
                {
                    string tmpStr = value.substr((equalPos + 1), (endPos - equalPos - 1));

                    tmpStr = trim(tmpStr, " ");
                    tmpStr = trim(tmpStr, "\"");

                    charsetStr = tmpStr;
                    printf("The charset is %s\n\n", charsetStr.c_str());

                    break;
                }

                iter++;
            }
        }
    }

    if (!charsetStr.empty())
    {
        return;
    }

    if (pNode == NULL)
    {
        return;
    }

    RecurseGetCharset(pNode->rightNode, charsetStr);
    RecurseGetCharset(pNode->leftNode, charsetStr);    
}

string WebPageParser::GetTitleFromTree()
{
    string retTitle = "";

    RecurseGetTitle(mStartNode, retTitle);

    RemoveHtmlCharacterEntities(retTitle);
    TrimContent(retTitle);

    if (mCharset != "UTF-8")
    {
        //printf("Need to transit code from %s to UTF-8!\n", mCharset.c_str());
        //retTitle = TransCodeIntoUtf8WithIconv(retTitle, mCharset);
    }
    
    //printf("The title is %s\n\n", retTitle.c_str());

    return retTitle;
}

void WebPageParser::RecurseGetTitle(Node* pNode, string& title)
{
    if (!title.empty())
    {
        return;
    }

    if ((pNode != NULL) && 
        (pNode->upperNode != NULL) && 
        (pNode->upperNode->type == TAG_START) && 
        (pNode->upperNode->tag_name == "title") && 
        (pNode->upperNode->rightNode == pNode) &&
        (pNode->type == CONTENT))
    {
        title = pNode->content;
    }

    if (pNode == NULL)
    {
        return;
    }

    RecurseGetTitle(pNode->rightNode, title);
    RecurseGetTitle(pNode->leftNode, title);
}

void WebPageParser::RecurseRemoveNode(Node* pNode)
{
    if (pNode == NULL)
    {
        return;
    }

    RecurseRemoveNode(pNode->leftNode);
    RecurseRemoveNode(pNode->rightNode);

    delete pNode;
}
