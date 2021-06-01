#include <iostream>
#include <windows.h>
#include <queue>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <array>
#include <string>
#include <cstring>
#include <atlstr.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <ctype.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <stringapiset.h>
#include <climits>
using namespace std;

const unordered_set<char> disallowedFilenameChars{ '<', '>', ':', '"', '/', '\\', '|', '?', '*', '\0' };

enum class Mode
{
    CompareFileNamesOnly,
    CompareFileSizesOnly,
    CompareHashes
};

class Options
{
public:
    string root;
    string output;
    unordered_set<string> excludeSubfolders;
    unordered_set<string> excludeFilenames;
    unordered_set<string> includeExtensions;
    unordered_set<string> excludeExtensions;
    unsigned long long fileSizeMax = ULLONG_MAX;
    unsigned long long fileSizeMin = 1;
    Mode mode = Mode::CompareHashes;
};

bool folderOrFileNameIsValid(string name)
{
    for (const char& c : name)
    {
        if (disallowedFilenameChars.find(c) != disallowedFilenameChars.end())
            return false;
    }

    return true;
}

class FileData
{
public:
    string name;
    string hash;
    string fileName;
    unsigned long long size;
    WIN32_FIND_DATAA data;

    FileData(string name, string fileName, string hash, WIN32_FIND_DATAA data, unsigned long long size) : name(name), hash(hash), size(size), data(data), fileName(fileName) {}
    FileData(FileData& o) : name(o.name), hash(o.hash), size(o.size), data(o.data), fileName(o.fileName) { }
    FileData(FileData&& o) noexcept : name(o.name), hash(o.hash), size(o.size), data(o.data), fileName(o.fileName) { }
};

LPCWSTR s2ws(const string& s)
{
    wstring stemp = wstring(s.begin(), s.end());
    LPCWSTR sw = stemp.c_str();

    return sw;
}

string convertFileNameToString(TCHAR* name)
{
    return string(CT2CA(CString(name)));
}

string computeHash(const char* file, WIN32_FIND_DATAA info)
{
    const size_t BUFFER_SIZE = 32768;

    HANDLE openReadHandle = CreateFileA(file, GENERIC_READ, 1, NULL, OPEN_EXISTING, info.dwFileAttributes, NULL);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    void* buffer = malloc(BUFFER_SIZE);
    DWORD bytesRead;
    while (true)
    {
        if (!ReadFile(openReadHandle, buffer, BUFFER_SIZE, &bytesRead, NULL))
        {
            std::cout << file << "\t FILE READ ERROR: " << GetLastError() << endl;
            free(buffer);
            return "ERROR";
        }
        else
        {
            if (bytesRead)
            {
                SHA256_Update(&sha256, buffer, bytesRead);
            }
            else
            {
                SHA256_Final(hash, &sha256);
                break;
            }
        }
    }

    free(buffer);

    stringstream result;

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        result << hex << setw(2) << setfill('0') << int(hash[i]);

    CloseHandle(openReadHandle);

    return result.str();
}

inline void unsafeAddToSet(unordered_set<string>& container, string item, bool isDirectory)
{
    if (isDirectory)
    {
        if (!folderOrFileNameIsValid(item))
            throw runtime_error(item + " is not a valid file or directory name.");
    }
    else
    {
        if (item[0] != '.' || item.length() == 1 || !folderOrFileNameIsValid(item))
            throw runtime_error(item + " is not a valid extension.");
    }

    container.insert(item);
}

void splitDelimitedParamItem(unordered_set<string>& container, const char* paramString, bool isDirectory)
{
    size_t strPos = 0;
    string item;
    while (true)
    {
        if (paramString[strPos] == ';')
        {
            unsafeAddToSet(container, item, isDirectory);
            item.clear();
        }
        else if (paramString[strPos] == '\0')
        {
            if (!item.empty())
                unsafeAddToSet(container, item, isDirectory);
            break;
        }
        else
            item += paramString[strPos];

        ++strPos;
    }
}

bool isValidNumber(const char* number)
{
    size_t index = 0;

    while (number[index] != '\0')
    {
        if (!isdigit(number[index]))
            return false;

        ++index;
    }

    return true;
}

unsigned long long cstringToUll(char* number)
{
    if (!isValidNumber(number))
        throw runtime_error(string(number) + " is not a valid number.");

    return strtoull(number, &number + strlen(number), 10);
}

inline bool setContainsLower(unordered_set<string>& container, char* item)
{
    return container.find(item) != container.end();
}

inline void stringToLower(char* s)
{
    size_t strPos = 0;
    while (s[strPos] != '\0')
    {
        s[strPos] = tolower(s[strPos]);
        ++strPos;
    }
}

inline bool setContainsLower(unordered_set<string>& container, string& item)
{
    return container.find(item) != container.end();
}

inline bool checkExtension(unordered_set<string>& container, string file, bool negate, bool empty)
{
    if (empty)
        return true;

    string extension;

    size_t indexOfDot = file.find_last_of('.');

    if (indexOfDot != -1)
        extension = file.substr(indexOfDot);

    return setContainsLower(container, extension) ^ negate;
}

inline unsigned long long nameFirstHash(char* fileName, unsigned long long length)
{
    return strlen(fileName);
}

inline unsigned long long lengthAndHashFirstHash(char* fileName, unsigned long long length)
{
    return length;
}

inline string nameSecondHash(FileData* data, unsigned long long length)
{
    string name(data->fileName);
    return name;
}

inline string lengthSecondHash(FileData* data, unsigned long long length)
{
    return to_string(length);
}

inline string hashSecondHash(FileData* data, unsigned long long length)
{
    return computeHash(data->name.c_str(), data->data);
}



Options getParams(int argc, char** args)
{
    Options* options = new Options();

    for (size_t i = 1; i < argc; ++i)
    {
        int charPos = 0;
        while (args[i][charPos] != '\0')
        {
            args[i][charPos] = tolower(args[i][charPos]);
            ++charPos;
        }

    }

    for (size_t i = 1; i < argc; i++)
    {
        if (strcmp(args[i], "-i") == 0)
        {
            if (!PathFileExistsA(args[++i]))
                throw runtime_error("Root folder doesn't exist.");

            options->root = args[i];
        }
        else if (strcmp(args[i], "-o") == 0)
        {
            char* outputParent = new char[MAX_PATH];
            strcpy(outputParent, args[++i]);
            PathRemoveFileSpecA(outputParent);
            if (!PathFileExistsA(outputParent))
                throw runtime_error("Output path doesn't exist.");

            char* rawFileName = new char[MAX_PATH];
            strcpy(rawFileName, args[i]);
            PathStripPathA(rawFileName);

            string rawFileNameString(rawFileName);
            if (!folderOrFileNameIsValid(rawFileNameString))
                throw runtime_error("Output filename " + rawFileNameString + " contains invalid characters.");

            options->output = args[i];

            delete[] outputParent;
            delete[] rawFileName;
        }
        else if (strcmp(args[i], "-exd") == 0)
            splitDelimitedParamItem(options->excludeSubfolders, args[++i], true);
        else if (strcmp(args[i], "-exf") == 0)
            splitDelimitedParamItem(options->excludeFilenames, args[++i], true);
        else if (strcmp(args[i], "-ine") == 0)
            splitDelimitedParamItem(options->includeExtensions, args[++i], false);
        else if (strcmp(args[i], "-exe") == 0)
            splitDelimitedParamItem(options->excludeExtensions, args[++i], false);
        else if (strcmp(args[i], "-smax") == 0)
            options->fileSizeMax = cstringToUll(args[++i]);
        else if (strcmp(args[i], "-smin") == 0)
            options->fileSizeMin = cstringToUll(args[++i]);
        else if (strcmp(args[i], "-mode") == 0)
        {
            if (strcmp(args[++i], "name") == 0)
                options->mode = Mode::CompareFileNamesOnly;
            else if (strcmp(args[i], "size") == 0)
                options->mode = Mode::CompareFileSizesOnly;
            else
                options->mode = Mode::CompareHashes;
        }
        else
        {
            throw runtime_error("Unrecognized parameter: " + string(args[i]));
        }
    }

    if (options->root.empty())
        throw runtime_error("Root folder must be specified.");
    if (options->output.empty())
        throw runtime_error("Output file must be specified.");
    if (options->includeExtensions.size() != 0 && options->excludeExtensions.size() != 0)
        throw runtime_error("Parameters -ine and -exe cannot be used together.");

    if (options->root[options->root.length() - 1] != '\\')
        options->root += '\\';

    Options returnValue(*options);
    delete options;
    return returnValue;
}

/*
* Params: 
* -i: Root folder*
* -o: output file name*
* -exd: exclude subfolders (delimited by ;. all subfolders will be included if null)
* -exf: exclude filenames (delimited by ;. All files will be included if null unless negated by a rule below)
* -ine: include extensions (delimited by ; WITH DOTS. If null, all extensions will be included. Providing a list of extensions will disregard everything else.
* -exe: exclude extensions (delimited by ; WITH DOTS. If null, no extensions will be excluded. Cannot be used together with -ine.
* -smax: ignore files greater than a size in bytes (if null, size limit is infinite).
* -smin: ignore files smaller than a size in bytes (if null, size limit is one byte).
* -mode: name - only compare file names, size - only compare file size, hash *default* - compare hashes if sizes are equal.
*/
int main(int argc, char* args[])
{
    Options options;
    try
    {
        options = getParams(argc, args);
    }
    catch (runtime_error& e)
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, 4);
        std::cout << "ERROR: "; 
        SetConsoleTextAttribute(hConsole, 15);
        std::cout << e.what() << endl;
    }
    
    time_t startTimeStamp = time(nullptr);

    WIN32_FIND_DATAA ffd;
    LARGE_INTEGER filesize;
    string rawDir, searchDir, fullName, extension;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    const DWORD ALLOWED_FILE_FLAGS = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ARCHIVE;

    unordered_map<unsigned long long, vector<FileData*>> firstHash;
    unordered_map<string, vector<FileData*>> secondHash;
    queue<string> folders;

    folders.push(options.root);

    unsigned long long files = 0;
    unsigned long long bytes = 0;
    unsigned long long folderCount = 1;
    auto firstHashFunction = lengthAndHashFirstHash;
    auto secondHashFunction = hashSecondHash;
    unordered_set<string>* extensionSet = nullptr;
    bool negateExtensionSetCheck = false;
    bool emptyExtensionSets = false;

    switch (options.mode)
    {
        case Mode::CompareFileNamesOnly:
            firstHashFunction = nameFirstHash;
            secondHashFunction = nameSecondHash;
            break;
        case Mode::CompareFileSizesOnly:
            secondHashFunction = lengthSecondHash;
            break;
    }

    if (options.excludeExtensions.size() > 0)
    {
        extensionSet = &options.excludeExtensions;
        negateExtensionSetCheck = true;
    }
    else if (options.includeExtensions.size() > 0)
    {
        extensionSet = &options.includeExtensions;
        negateExtensionSetCheck = false;
    }
    else
        emptyExtensionSets = true;

    while (!folders.empty())
    {
        rawDir = folders.front();
        folders.pop();

        std::cout << rawDir << endl;

        if (rawDir.length() > (MAX_PATH - 3))
        {
            std::cout << "Directory path " << rawDir << " is too long.\n" << endl;
            continue;
        }

        searchDir = rawDir + "*";

        hFind = FindFirstFileA(searchDir.c_str(), &ffd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                stringToLower(ffd.cFileName);
                fullName = rawDir + ffd.cFileName;

                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && ffd.cFileName[0] != '.' && ffd.cFileName[1] != '\0' && ffd.cFileName[0] != '.' && ffd.cFileName[1] != '.' && ffd.cFileName[2] != '\0')
                {
                    if (!setContainsLower(options.excludeSubfolders, ffd.cFileName))
                    {
                        folders.push(fullName + "\\");
                        ++folderCount;
                    }
                }
                else if ((ffd.dwFileAttributes & ALLOWED_FILE_FLAGS) && checkExtension(*extensionSet, ffd.cFileName, negateExtensionSetCheck, emptyExtensionSets) && !setContainsLower(options.excludeFilenames, ffd.cFileName))
                {
                    filesize.LowPart = ffd.nFileSizeLow;
                    filesize.HighPart = ffd.nFileSizeHigh;
                    bytes += filesize.QuadPart;
                    ++files;

                    if (filesize.QuadPart >= options.fileSizeMin && filesize.QuadPart <= options.fileSizeMax)
                    {
                        FileData* data = new FileData(fullName, ffd.cFileName, "", ffd, filesize.QuadPart);
                        unsigned long long firstHashKey;

                        firstHashKey = firstHashFunction(ffd.cFileName, filesize.QuadPart);

                        firstHash[firstHashKey].push_back(data);

                        if (firstHash[firstHashKey].size() >= 2)
                        {
                            for (size_t i = 0; i < firstHash[firstHashKey].size(); ++i)
                            {
                                FileData* temp = firstHash[firstHashKey][i];
                                if (temp->hash == "")
                                {
                                    temp->hash = secondHashFunction(temp, filesize.QuadPart);

                                    secondHash[temp->hash].push_back(temp);
                                }
                            }
                        }
                    }
                }
            } while (FindNextFileA(hFind, &ffd) != 0);

            FindClose(hFind);
        }
    }
    time_t endTimeStamp = time(nullptr);

    ofstream file(options.output);
    file << "-----TDFF RESULT FILE-----" << endl;
    
    file << "Mode: ";
    switch (options.mode)
    {
        case Mode::CompareFileNamesOnly:
            file << "CompareFileNamesOnly";
            break;
        case Mode::CompareFileSizesOnly:
            file << "CompareFileSizesOnly";
            break;
        default:
        case Mode::CompareHashes:
            file << "CompareHashes";
            break;
    }
    file << endl;

    file << "Root folder: " << options.root << endl;
    file << "Output location: " << options.output << endl;
    file << "Minimum file size: " << options.fileSizeMin << " bytes" << endl;
    file << "Maximum file size: " << options.fileSizeMax << " bytes" << endl;
    file << "Start time: " << asctime(localtime(&startTimeStamp));
    file << "End time: " << asctime(localtime(&endTimeStamp));
    file << "Folders scanned: " << folderCount << endl;
    file << "Files scanned: " << files << endl;
    file << "Bytes scanned: " << bytes << endl;

    file << "Extensions excluded: ";
    size_t tempIndex = 0;
    if (options.excludeExtensions.size() == 0)
        file << "NONE";
    else
    {
        for (const string& ext : options.excludeExtensions)
        {
            file << ext;
            if (tempIndex++ <= options.excludeExtensions.size())
                file << ", ";
        }
    }
    file << endl;

    file << "Extensions included (except those explicitly excluded): ";
    tempIndex = 0;
    if (options.includeExtensions.size() == 0)
        file << "*";
    else
    {
        for (const string& ext : options.includeExtensions)
        {
            file << ext;
            if (tempIndex++ <= options.includeExtensions.size())
                file << ", ";
        }
    }
    file << endl;

    file << "Folders excluded: ";
    tempIndex = 0;
    if (options.excludeSubfolders.size() == 0)
        file << "NONE";
    else
    {
        for (const string& ext : options.excludeSubfolders)
        {
            file << ext;
            if (tempIndex++ != options.excludeSubfolders.size() - 1)
                file << ", ";
        }
    }
    file << endl;

    file << "File names excluded: ";
    tempIndex = 0;
    if (options.excludeFilenames.size() == 0)
        file << "NONE";
    else
    {
        for (const string& ext : options.excludeFilenames)
        {
            file << ext;
            if (tempIndex++ <= options.excludeFilenames.size())
                file << ", ";
        }
    }
    file << endl << endl;

    unsigned long long duplicatesSizeTotal = 0, count = 0;

    for (const auto& size : secondHash)
    {
        if (size.second.size() > 1)
        {
            file << "GROUP" << endl;
            count += size.second.size() - 1;
            duplicatesSizeTotal += (size.second[0]->size * (size.second.size() - 1));
            for (size_t i = 0; i < size.second.size(); ++i)
                file << "FILE " << size.second[i]->name << endl;

        }
    }

    file << endl;

    file << "Total duplicates: " << count << endl;
    file << "Total duplicates size: " << duplicatesSizeTotal << endl;

    file.close();

    std::cout << endl << "Done!" << endl;
}