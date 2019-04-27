#include <stdio.h>
#include <string.h>
#include <windows.h>

#include <string>
#include <vector>
using namespace std;

//#include <shlwapi.h>
//#pragma comment(lib, "shlwapi.lib")

static char g_NameBuf[260];
static char g_Fout[260] = "enumdir.txt.xml";
static char * g_filters[] = {".c",".cpp",".cxx",".cc",".cx",".h",".hpp",".hxx",".hh",".hx",".txt",".ini",".cfg",".lua",".py",".pl",".sh",".cmd",".bat",""};
static char * g_bls[] = {".git",""};

typedef bool (WINAPI *EnumerateFunc) (const char* lpFileOrPath, void* pUserData);  

void doFileEnumeration(char* lpPath, bool bRecursion, bool bEnumFiles, FILE * f, int indent, EnumerateFunc pFileFunc, void* pFileData, EnumerateFunc pDirFunc, void* pDirData) 
{  
    static bool s_bUserBreak = false;
    if(s_bUserBreak) return;

    char path[260] = {0};
    size_t len = strlen(lpPath);
    if(len >= 256 || len <= 0) return;        
    strcpy(path, lpPath);  
    if(lpPath[len-1] != '\\')
        strcat(path, "\\*");
    else
        strcat(path, "*");

    WIN32_FIND_DATA fd;  
    HANDLE hFindFile = FindFirstFile(path, &fd);  
    if(hFindFile == INVALID_HANDLE_VALUE)  
    {  
        FindClose(hFindFile);
        return;
    }

    bool bFinish = false;
    bool bUserReture=true;
    bool bIsDirectory; 
    char tempPath[260] = {0};
    vector<string> vFiles;        

    while(!bFinish) {  
        strcpy(tempPath, lpPath);  
        if(lpPath[len-1] != '\\') {
            strcat(tempPath, "\\");
        }
        strcat(tempPath, fd.cFileName);

        bIsDirectory = ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        if( bIsDirectory && (0 == strcmp(fd.cFileName, ".")) || (0 == strcmp(fd.cFileName, ".."))) {         
            bFinish = !FindNextFile(hFindFile, &fd);  
            continue;
        }

        if(bEnumFiles && !bIsDirectory) {
            if(pFileFunc) {  
                bUserReture = pFileFunc(tempPath, pFileData);  
                if(bUserReture) {  
                    vFiles.push_back(fd.cFileName);
                }
            } else {
               vFiles.push_back(fd.cFileName);
            }
        }

        
        if(bIsDirectory && bRecursion) {
            if(pDirFunc) {
                bUserReture = pDirFunc(tempPath, pDirData);               
            } else {
                bUserReture = false;
            }
        }

        if(bIsDirectory && bRecursion && !bUserReture) {
           for(int i = 0; i < indent; i++) {
                fprintf(f, "\t");
            }
            fprintf(f, "<Filter Name=\"%s\">\n", fd.cFileName);
            doFileEnumeration(tempPath, bRecursion, bEnumFiles, f, indent+1, pFileFunc, pFileData, pDirFunc, pDirData);
            for(int i = 0; i < indent; i++) {
                fprintf(f, "\t");
            }
            fprintf(f, "</Filter>\n", fd.cFileName);
        }

        bFinish = !FindNextFile(hFindFile, &fd);
    }

    path[strlen(path)-1] = '\x00';
    for(vector<string>::iterator it = vFiles.begin(); it != vFiles.end(); it++) {
        for(int i = 0; i < indent; i++) {
            fprintf(f, "\t");
        }
        fprintf(f, "<File RelativePath=\"%s%s\">\n", path, it->c_str());
        for(int i = 0; i < indent; i++) {
            fprintf(f, "\t");
        }
        fprintf(f, "</File>\n");
    }

    FindClose(hFindFile);
}

bool WINAPI myEnumerateFileFunc(const char* lpFileOrPath, void * pUserData)
{
    //vector<string> vFilters;
    if(NULL == pUserData)
        return true;

    if(NULL == lpFileOrPath)
        return false;
    size_t slen = strlen(lpFileOrPath);
    if(slen < 4) {
        return false;
    }
    size_t offs;
    char ** lpFilters = (char**)pUserData;    
    while(offs = strlen(*lpFilters)) {         
        if(offs <= 4 && 0 == stricmp(&lpFileOrPath[slen-offs], *lpFilters)) {
           //vFilters.push_back(*lpFilters);
            return true;
        }
        lpFilters++;
    }

    return false;
}

bool WINAPI myEnumerateDirFunc(const char* lpFileOrPath, void * pUserData)
{
    //vector<string> vFilters;    
    if(NULL == pUserData)
        return false;

    if(NULL == lpFileOrPath)
        return false;
    size_t slen = strlen(lpFileOrPath);
    if(slen < 2) {
        return false;
    }
    size_t offs;
    char ** lpFilters = (char**)pUserData;    
    while(offs = strlen(*lpFilters)) {         
        if(0 == stricmp(&lpFileOrPath[slen-offs], *lpFilters)) {
           //vFilters.push_back(*lpFilters);
            return true;
        }
        lpFilters++;
    }

    return false;
}

int main(int argc, char * argv[])
{
    if(argc < 2) {
        if(GetCurrentDirectory(260, g_NameBuf) >= 256) {
            printf("GetCurrentDirectory error!\n");
            return -1;
        }
    } else {
        if('\"' == argv[1][0] || NULL != strstr(argv[1], " ")) {
            printf("argv[1] error: format!\n");
            return -2;
        }
        int n = strlen(argv[1]);
        if(n >= 256) {
            printf("argv[1] error: strlen!\n");
            return -3; 
        }
        strcpy(g_NameBuf, argv[1]);
        if('\\' == g_NameBuf[n-1]) {
            g_NameBuf[n-1] = 0;
        }
    }

    if(argc >= 3 && strlen(argv[2]) < 256) {
        strcpy(g_Fout, argv[2]);
        strcat(g_Fout, ".xml");
    }

    const char * p = strrchr(g_Fout, '\\');
    if(NULL == p) {
        p = strrchr(g_Fout, '/');
        if(NULL == p) {
            p = g_Fout;
        } else {
            p = p + 1;
        }
    } else {
        const char * q = strrchr(p + 1, '/');
        if(NULL == q) {
            p = p + 1;           
        } else {
            p = q + 1;
        }
    }  

    //if(!PathFileExists(g_NameBuf)) {
    //    printf("PathFileExists error!\n");
    //    return -4;
    //}    

    FILE * fout = fopen(p, "w+");
    if(NULL == fout) {
        printf("fopen error!\n");
        return -5;
    }
    fprintf(fout, "\t<Files>\n");
    doFileEnumeration(g_NameBuf, true, true, fout, 2, 
        myEnumerateFileFunc,
        g_filters,
        myEnumerateDirFunc,
        g_bls);
    fprintf(fout, "\t</Files>\n");
    fclose(fout);

    return 0;
}  
