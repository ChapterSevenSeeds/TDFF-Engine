#include <iostream>
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

#include <ctime>
#include <fstream>
#include <stdexcept>
#include <ctype.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <stringapiset.h>
#include <climits>
#include <filesystem>
#include <io.h>
#include <fcntl.h>

#include "types.h";
#include "HashHelpers.h"
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
        s[strPos] = char(tolower(s[strPos]));
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

inline unsigned long long nameFirstHash(char* fileName, unsigned long long)
{
    return strlen(fileName);
}

inline unsigned long long lengthAndHashFirstHash(char*, unsigned long long length)
{
    return length;
}

//inline string nameSecondHash(FileData* data, unsigned long long)
//{
//    string name(data->fileName);
//    return name;
//}

//inline string lengthSecondHash(FileData*, unsigned long long length)
//{
//    return to_string(length);
//}

//inline string hashSecondHash(FileData* data, unsigned long long)
//{
//    return computeHash(data->name.c_str(), data->data);
//}



Options getParams(int argc, char** args)
{
    Options* options = new Options();

    for (size_t i = 1; i < argc; ++i)
    {
        int charPos = 0;
        while (args[i][charPos] != '\0')
        {
            args[i][charPos] = char(tolower(args[i][charPos]));
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
    HashHelpers hashHelpers;
    /*Options options;
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
    
    time_t startTimeStamp = time(nullptr);*/
    const string NO_HASH = "nohash";

    unordered_map<ulong, unordered_map<string, vector<string>>> results;
    _setmode(_fileno(stdout), _O_U16TEXT);
    auto folders = queue<filesystem::directory_entry>();
    folders.push(filesystem::directory_entry{ "C:/Users/Tyson/Desktop/Blender/Spanish Project/Models" });
    try 
    {
        while (!folders.empty()) 
        {
            for (auto const& dir_entry : filesystem::directory_iterator{ folders.front() })
            {
                if (dir_entry.is_directory()) 
                {
                    folders.push(dir_entry);
                }
                else if (dir_entry.is_regular_file()) 
                {
                    if (results[dir_entry.file_size()].contains(NO_HASH) && results[dir_entry.file_size()][NO_HASH].size() >= 1)
                    {
                        results[dir_entry.file_size()][NO_HASH].push_back(dir_entry.path().string());
                        for (auto& file : results[dir_entry.file_size()][NO_HASH]) 
                        {
                            results[dir_entry.file_size()][hashHelpers.computeHash(file)].push_back(file);
                        }

                        results[dir_entry.file_size()].erase(NO_HASH);
                    }
                }
            }
            folders.pop();
        }
    }
    catch (exception e)
    {
        cout << e.what();
    }

    /*time_t endTimeStamp = time(nullptr);

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

    std::cout << endl << "Done!" << endl;*/
}