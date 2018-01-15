#ifndef _WEBPAGEPARSER_H_
#define _WEBPAGEPARSER_H_

#include "Common.h"

class WebPageParser
{
public:
    enum NodeType
    {
        TAG_START = 1,
        TAG_END,
        CONTENT,
        COMMENTS
    };

    struct Node
    {
        NodeType type; 
        bool meetEnd;
        string tag_name;
        map<string, string> tag_attrs;
        string content;
        Node* upperNode;
        Node* leftNode;
        Node* rightNode;
    };

public:
    WebPageParser(std::string homeUrl, std::string filePath);
    ~WebPageParser();

    string FoundContent(bool levelFlag = false);

    string GetCharset() { return mCharset; }
    string GetTitle() { return mTitle; }
    set<string>* GetFouldUrlsSet() { return &mFoundUrls; }

protected:
    string mHomeUrl;
    string mWebPage;

    string mCharset;
    string mTitle;
    set<string> mFoundUrls;

    Node* mStartNode;
    Node* mCurrentNode;

protected:
    void ReadWebPageFromFile(string filePath);

    void BuildTree();    
    WebPageParser::Node* InitNewNode(NodeType type);
    void PutNewNodeIntoTree(Node* newNode);

    void CheckTree(Node* pNode);

    bool IsTagStart(string str);
    bool IsTagEnd(string str);

    WebPageParser::NodeType JudgeStatus(int curPos);

    // Process Tag Start
    bool ProcessTagStart(int& curPos);
    void ParseTagBlock(Node* pNode, bool isTagName, string block);
    bool CheckTagSpecialEnd(Node* pNode, int& curPos);
    void ProcessTagScript(Node* pNode, int& curPos);

    // Process Tag End
    bool ProcessTagEnd(int& curPos);
    Node* FindUpperNotEndNode(Node* curNode);
    bool CheckAndSetUpperNodeEnd(Node* curNode, string tagName);

    // Process Content
    bool ProcessContent(int& curPos);
    void TrimContent(string& content);

    // Get all the urls in the webpage
    void FoundAllUrls();
    void RecurseSearchUrl(Node* pNode);

    // Get pure content    
    void RecurseGetDivWithMaxP(Node* pNode, Node*& pDivWithMaxP, int& tagPCounter);
    void RecurseCheckDiv(Node* pNode, int& tagPCounter, int& tagACounter, int& contentLen);
    bool IsTagPBelowNode(Node* pNode);
    Node* FindClosestFatherNode(Node* pNode);
    void RecurseGetContent(Node* pNode, string& cumStr, bool levelFlag = false);
    //int GetNodeLevel(Node* pNode);
    string GetNodeLevel(Node* pNode);
    void RemoveHtmlCharacterEntities(string& content);

    // Get charset
    string GetCharsetFromTree();
    void RecurseGetCharset(Node* pNode, string& charsetStr);

    // Get title
    string GetTitleFromTree();
    void RecurseGetTitle(Node* pNode, string& title);

    void RecurseRemoveNode(Node* pNode);
};

#endif
