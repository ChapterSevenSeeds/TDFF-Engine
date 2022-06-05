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
#include "helpers.h";
using namespace std;







//void splitDelimitedParamItem(unordered_set<string>& container, const char* paramString, bool isDirectory)
//{
//    size_t strPos = 0;
//    string item;
//    while (true)
//    {
//        if (paramString[strPos] == ';')
//        {
//            unsafeAddToSet(container, item, isDirectory);
//            item.clear();
//        }
//        else if (paramString[strPos] == '\0')
//        {
//            if (!item.empty())
//                unsafeAddToSet(container, item, isDirectory);
//            break;
//        }
//        else
//            item += paramString[strPos];
//
//        ++strPos;
//    }
//}

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

    unordered_map<ulong, unordered_map<string, vector<wstring>>> results;
    _setmode(_fileno(stdout), _O_U16TEXT);
    auto folders = queue<filesystem::directory_entry>();
    folders.push(filesystem::directory_entry{ "C:/Users/Tyson/Pictures" });
    try 
    {
        while (!folders.empty()) 
        {
            filesystem::directory_entry currentDirectory = folders.front();
            folders.pop();
            wcout << currentDirectory << '\n';
            for (auto const& dir_entry : filesystem::directory_iterator{ currentDirectory })
            {
                if (dir_entry.is_directory()) 
                {
                    folders.push(dir_entry);
                }
                else if (dir_entry.is_regular_file()) 
                {
                    ulong fileSize = dir_entry.file_size();
                    wstring fileName = dir_entry.path().wstring();
                    if (results.contains(fileSize))
                    {
                        unordered_map<string, vector<wstring>>& sizeMap = results[fileSize];
                        if (sizeMap.contains(NO_HASH) && sizeMap[NO_HASH].size() >= 1)
                        {
                            for (wstring& file : sizeMap[NO_HASH])
                            {
                                sizeMap[hashHelpers.computeHash(file)].push_back(file);
                            }

                            sizeMap.erase(NO_HASH);
                        }

                        sizeMap[hashHelpers.computeHash(fileName)].push_back(fileName);
                    }
                    else
                    {
                        results[fileSize][NO_HASH].push_back(fileName);
                    }
                }
            }
        }
    }
    catch (exception e)
    {
        cout << e.what();
    }

    for (const auto& sizeMap : results) 
    {
        for (const auto& hashMap : sizeMap.second) 
        {
            for (const auto& file : hashMap.second)
            {
                wcout << "DUPLICATE: " << file << '\n';
            }
        }
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