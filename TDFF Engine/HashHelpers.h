#pragma once
#include <fstream>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include "types.h"

using namespace std;

class HashHelpers
{
    char* buffer;
    size_t bufferSize;

public:
    HashHelpers(size_t bufferSize = 32768)
    {
        this->bufferSize = bufferSize;
        buffer = reinterpret_cast<char*>(malloc(bufferSize));
    }

    ~HashHelpers() 
    {
        free(buffer);
    }
    string computeHash(wstring& file)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);

        ulong bytesRead = 0;
        ifstream stream(file);
        do
        {
            stream.read(buffer, bufferSize);
            bytesRead = stream.gcount();
            if (bytesRead)
            {
                SHA256_Update(&sha256, buffer, bytesRead);
            }
            else
            {
                SHA256_Final(hash, &sha256);
                break;
            }
        } while (bytesRead);

        stream.close();

        stringstream result;

        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
            result << hex << setw(2) << setfill('0') << int(hash[i]);

        return result.str();
    }
};