#include "Common.h"
#include "WebPageParser.h"

void ParseParams(int argc, char *argv[])
{
	if (argc > 1)
	{
		int result = 0;

		while ((result = getopt(argc, argv, "p:n:i:u:")) != -1)
		{
			switch (result)
			{
				case 'p': // output path
					gOutputPath = string(optarg);
					break;

				case 'n': // base name
					gBaseName = string(optarg);
					break;

				case 'i': // input html file
					gInputHtmlFile = string(optarg);
					break;

				case 'u': // url
					{
						string url = string(optarg);
						gHomeUrl = GetHomeUrl(url);
					}

					break;

				default:
					break;
			}
		}
	}

	//printf("gOutputPath: %s\ngBaseName: %s\ngInputHtmlFile: %s\ngHomeUrl: %s\n", 
		//gOutputPath.c_str(), gBaseName.c_str(), gInputHtmlFile.c_str(), gHomeUrl.c_str());

	if (gOutputPath.empty() || gBaseName.empty() || gInputHtmlFile.empty() || gHomeUrl.empty())
	{
		printf("Some parameters are not set! syntax: \"./parser -p outputpath -n basename -i inputhtml -u url\"\n");
		exit(1);
	}
}

void GlobalInit()
{
    signal(SIGPIPE, SIG_IGN);

	InitTagNames();
}

int main(int argc, char *argv[])
{
	// ./parser -p outputpath -n basename -i inputhtmlfile -u url
	ParseParams(argc, argv);
	
    GlobalInit();

	WebPageParser parser(gHomeUrl, gInputHtmlFile);

	string title = parser.GetTitle();
	//printf("Title: %s\n", title.c_str());

	string titleFileName = gOutputPath + "/" + gBaseName + ".title";
	FILE* pTitleFile = fopen(titleFileName.c_str(), "w");
	if (pTitleFile != NULL)
	{
		fputs(title.c_str(), pTitleFile);
		fputs("\n", pTitleFile);
		fclose(pTitleFile);
	}
	else
	{
		string errMsg = "Failed to open and write title file " + titleFileName + "\n";
		printf(errMsg.c_str());
	}

	string content = parser.FoundContent();
	//printf("Pure content:\n %s\n", content.c_str());

	/*
	string contentFileName = gOutputPath + "/" + gBaseName + ".content";
	FILE* pContentFile = fopen(contentFileName.c_str(), "w");
	if (pContentFile != NULL)
	{
		fputs(content.c_str(), pContentFile);
		fclose(pContentFile);
	}
	else
	{
		string errMsg = "Failed to open and write content file " + contentFileName + "\n";
		printf(errMsg.c_str());
	}
	*/

	string levelContent = parser.FoundContent(true);
	//printf("Pure content:\n %s\n", content.c_str());

	string levelContentFileName = gOutputPath + "/" + gBaseName + ".levelcontent";
	FILE* pLevelContentFile = fopen(levelContentFileName.c_str(), "w");
	if (pLevelContentFile != NULL)
	{
		fputs(levelContent.c_str(), pLevelContentFile);
		fclose(pLevelContentFile);
	}
	else
	{
		string errMsg = "Failed to open and write level content file " + levelContentFileName + "\n";
		printf(errMsg.c_str());
	}

	set<string>* pUrlSet = parser.GetFouldUrlsSet();
	//printf("URLs:\n");

	string linkFileName = gOutputPath + "/" + gBaseName + ".links";
	FILE* pLinkFile = fopen(linkFileName.c_str(), "w");
	if (pLinkFile != NULL)
	{
		set<string>::iterator iter = pUrlSet->begin();
		while (iter != pUrlSet->end())
		{
			//printf("%s\n", (*iter).c_str());

			fputs((*iter).c_str(), pLinkFile);
			fputs("\n", pLinkFile);

			iter++;
		}
		fclose(pLinkFile);
	}
	else
	{
		string errMsg = "Failed to open and write link file " + linkFileName + "\n";
		printf(errMsg.c_str());
	}

	printf("Parser finished\n");

    return 0;
}

