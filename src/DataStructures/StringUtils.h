#ifndef STRINGUTILS_H_
#define STRINGUTILS_H_

bool CheckFileExtension(char* a_filename, char* extension);  // ".jpg"
void GetFileFromFullPath(char* fullpath, bool removeextension = false);
int GetSomeColor(int i);

void RemoveCharactersFromString(char* iString, const char* Key = "-:/`(,); ");
void ReplaceCharactersWith(char* iString, const char* Key = "-:`(,);", const char* ReplaceChar = " ");
void ReplaceCharactersEntersTooWith(char* iString, const char* Key = "-:`(,);", const char* ReplaceChar = " ");
void RemoveStringsFromString(char* iString, const char* Key);
void ReplaceStringWithString(char* iString, const char* Key, const char* replace);

char* strstrfromend(const char* string, const char* stringtofind, int findlen = 0); // like strstr but it starts from the end of string
char* GetStringBetween(const char* string, const char* from, const char* to, char** ended = 0); // GetStringBetween("item1, (item2), item3","(",")") -> "item2"
void CutLastDir(char* fullpath);
int SearchLastPosition(const char* string, const char* stringtofind);
void CutFileFromFullPath(char* fullpath);
unsigned int NearestPow2(unsigned int aNum);

void SplineDeriv2(double* x, double* y, int n, double yp1, double ypn, double* y2);
void SplineInterpolate(double* ixInput, double *iyInput, int InputSN, double* ixOutput, double* iyOutput, int OutputSN);

void CutFileFormFullPath(char* fullpath);
void RemoveStringFromBegin(char* iString, const char*Key);
void RemoveStringFromEnd(char* iString, const char*Key);

void RemoveEnters(char* a_string);
char* findMatchingBracelet(char* aStr);
char nextNonSpace(char* aStr);
void ReplaceInplaceCharWithChar(char* aStr, const char aToReplace, const char aReplaceWith);
void ReplaceInplaceCharWithCharLen(char* aStr, unsigned int aReplacingLen, const char aToReplace, const char aReplaceWith);
void PreprocessComments(std::string& a_script);
void replace_all_between(std::string& a_str, const char* a_start, const char* a_stop);
void replace_all(std::string& str, const std::string& to_replace, const std::string& replace_with);

string urlEncode(string str);
string urlDecode(string str);

#endif // STRINGUTILS_H_
