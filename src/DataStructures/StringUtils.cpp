#include <string.h>
#include <stdio.h>
#include <iostream>
using namespace std;

#include "stringutils.h"

void ReplaceInplaceCharWithCharLen(char* aStr, unsigned int aReplacingLen, const char aToReplace, const char aReplaceWith)
{
    for (unsigned int i = 0; i < aReplacingLen; ++i)
        if (aStr[i] == aToReplace)
            aStr[i] = aReplaceWith;
}

void RemoveEnters(char* a_string)
{
    char key[3];
    key[0] = 13;
    key[1] = 10;
    key[2] = 0;
    ReplaceStringWithString(a_string, key, "");
}

char* findMatchingBracelet(char* aStr)
{
    char* nextBraceletClose = strstr(aStr, "}");
    char* nextBraceletOpen = strstr(aStr + 1, "{");
    if (!nextBraceletOpen || nextBraceletOpen > nextBraceletClose)
        return nextBraceletClose;

    int opencount = 0;
    int closecount = 0;
    int len = strlen(aStr);
    char* matchingBracelet = 0;
    for (int i = 1; i < len; ++i)
    {
        if (aStr[i] == '{')
            ++opencount;
        if (aStr[i] == '}')
            ++closecount;
        if (closecount > opencount)
        {
            matchingBracelet = aStr + i;
            break;
        }
    }
    return matchingBracelet;
}

char nextNonSpace(char* aStr)
{
    int len = strlen(aStr);
    for (int i = 1; i < len; ++i)
    {
        if (aStr[i] != ' ' && aStr[i] != '\r' && aStr[i] != '\n')
            return aStr[i];
    }
    return 0;
}

void ReplaceInplaceCharWithChar(char* aStr, const char aToReplace, const char aReplaceWith)
{
    unsigned int len = strlen(aStr);
    for (unsigned int i = 0; i < len; ++i)
        if (aStr[i] == aToReplace)
            aStr[i] = aReplaceWith;
}

string urlEncode(string str)
{
    string new_str = "";
    char c;
    int ic;
    const char* chars = str.c_str();
    char bufHex[10];
    int len = strlen(chars);

    for (int i = 0; i < len; i++)
    {
        c = chars[i];
        ic = c;
        if (false && c == ' ') // remove false condition to encode spaces with "+"
            new_str += '+';
        else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            new_str += c;
        else
        {
            sprintf(bufHex, "%X", c);
            if (ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}

string urlDecode(string str)
{
    string ret;
    char ch;
    int i, ii, len = str.length();

    for (i = 0; i < len; i++)
    {
        if (str[i] != '%')
        {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        }
        else
        {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return ret;
}

void replace_all_between(std::string& a_str, const char* a_start, const char* a_stop)
{
    size_t nFPos = a_str.find(a_start);
    while (nFPos != string::npos)
        if (size_t stop_pos = a_str.find(a_stop, nFPos))
        {
            a_str.erase(nFPos, stop_pos - nFPos);
            nFPos = a_str.find(a_start);
        }
        else
            break;
}

void replace_all(std::string& str, const std::string& to_replace, const std::string& replace_with)
{
    size_t start_pos = 0;
    while((start_pos = str.find(to_replace, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, to_replace.length(), replace_with);
        start_pos += replace_with.length(); /** in case 'replace_with' is substring of 'to_replace' */
    }
}

void PreprocessComments(std::string& a_script)
{
    ReplaceInplaceCharWithCharLen((char*)a_script.c_str(), a_script.length(), '\t', ' ');
    replace_all_between(a_script, "//", "\r\n");
    replace_all_between(a_script, "//", "\n");
    replace_all_between(a_script, "/*", "*/");
}
