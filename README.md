# Tyson's Duplcate File Finder Engine
### A Windows-only command line tool for finding duplicate files.

#### Arguments
| Argument | Description | Example | Default | Comments |
| -------- | ----------- | ------- | ------- | -------- |
| -i | Specifies the root folder. | C:\Users\Tyson | N/A | Required. Wrap the path in double quotes if it contains whitespace. Relative paths not supported. |
| -o | Specifies the output file path and name. | C:\Users\Tyson\Results.txt | N/A | Required. Wrap the path in double quotes if it contains whitespace. Relative paths not supported. Will be converted to lowercase. |
| -exd | Semicolon delimited names of subfolders to be excluded in the search. | node_modules;documents;pictures | None | Wrap the entire list in double quotes if any paths contain whitespace. |
| -exf | Semicolon delimited names of files to be excluded in the search. | desktop.ini;myface.png;hello.docx | None | Wrap the entire list in double quotes if any paths contain whitespace. |
| -ine | Semicolon delimited extensions (with the dot) to be searched for. | .js;.docx;.png | * | If this parameter is used, all other extensions not specified in the list will be ignored. |
| -exe | Semicolon delimited extensions (with the dot) to be excluded in the search. | .ts;.ini;.sys;.dll | None | This parameter cannot be used together with the previous parameter. |
| -smax | File size minimum in bytes. Ignores all files smaller than this size. | 50000 | 1 | None. |
| -smin | File size maximum in bytes. Ignores all files larger than this size. | 1000000 | 18446744073709551615 | None. |
| -mode | Specifies the type of file comparison. Can be one of three values: name, size, or hash. | name | hash | If the hash mode is selected, files will be compared first by size and then by SHA256 checksum. |

#### Notes
All comparisons are performed without regarding case since the Win32 API takes no regard to casing when searching for files.
A GUI for running this app and viewing its output is under development.
