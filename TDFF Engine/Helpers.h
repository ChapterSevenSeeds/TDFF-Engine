#pragma once
#include <unordered_set>
#include <string>
#include <vector>

using namespace std;

const unordered_set<char> disallowedFilenameChars{ '<', '>', ':', '"', '/', '\\', '|', '?', '*', '\0' };

bool folderOrFileNameIsValid(string name)
{
    for (const char& c : name)
    {
        if (disallowedFilenameChars.contains(c))
            return false;
    }

    return true;
}

vector<string> split(string input, string delimiter)
{
    vector<string> results;
    string delimiterReadBuffer;
    size_t indexInDelimiter = 0;
    string currentRun;

    for (const char& c : input)
    {
        if (c == delimiter[indexInDelimiter])
        {
            if (++indexInDelimiter == delimiter.length())
            {
                indexInDelimiter = 0;
                if (currentRun.length() > 0)
                {
                    delimiterReadBuffer.clear();
                    results.push_back(currentRun);
                    currentRun.clear();
                }
            }
            else
            {
                delimiterReadBuffer += c;
            }
        }
        else
        {
            if (delimiterReadBuffer.length() > 0)
            {
                currentRun += delimiterReadBuffer;
                indexInDelimiter = 0;
                delimiterReadBuffer.clear();
            }

            currentRun += c;
        }
    }

    if (delimiterReadBuffer.length() > 0)
    {
        currentRun += delimiterReadBuffer;
    }

    if (currentRun.length() > 0) results.push_back(currentRun);

    return results;
}