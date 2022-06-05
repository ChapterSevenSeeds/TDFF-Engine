#pragma once
#include <string>
#include <unordered_set>
#include <filesystem>
#include "helpers.h"
#include "types.h"

using namespace std;
class Options
{
private:
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

    inline void extendUnorderedSet(unordered_set<string>& container, vector<string> items)
    {
        for (const auto& item : items) container.insert(item);
    }

    inline vector<string>& checkPathItems(vector<string>& items, string& itemType)
    {
        for (const auto& item : items) 
        {
            if (!folderOrFileNameIsValid(item)) throw runtime_error("\"item\" is not a valid " + itemType + ".");
        }

        return items;
    }
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

    static Options& parseFromArgs(int argc, char** args)
    {
        Options options;

        for (size_t i = 1; i < argc; i++)
        {
            if (strcmp(args[i], "-i") == 0)
            {
                if (!filesystem::exists(args[++i]))
                    throw runtime_error("Root folder doesn't exist.");

                options.root = args[i];
            }
            else if (strcmp(args[i], "-o") == 0)
            {
                filesystem::directory_entry output(args[++i]);
                if (!filesystem::exists(output.path().parent_path()))
                    throw runtime_error("Output path doesn't exist.");

                if (!folderOrFileNameIsValid(output.path().string()))
                    throw runtime_error("Output filename " + output.path().string() + " contains invalid characters.");

                options.output = args[i];
            }
            else if (strcmp(args[i], "-exd") == 0)
            {
                splitDelimitedParamItem(options->excludeSubfolders, args[++i], true);
            }
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
};