# Tyson's Duplcate File Finder Engine
## A Windows-only command line tool for finding duplicate files.

### Arguments
| Argument | Description | Example | Comments |
| -------- | ----------- | ------- | -------- |
| -i | Specifies the root folder. | C:\Users\Tyson | Required. Wrap the path in double quotes if it contains whitespace. Relative paths not supported. |
| -o | Specifies the output file path and name. | C:\Users\Tyson\Results.txt | Required. Wrap the path in double quotes if it contains whitespace. Relative paths not supported. Will be converted to lowercase. |
| -exd | Semicolon delimited names of subfolders to be excluded in the search.. | node_modules;documents;pictures | Wrap the entire list in double quotes if any paths contain whitespace. |
| 
