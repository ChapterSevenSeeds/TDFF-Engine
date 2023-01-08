# Tyson's duplicate file finder engine
A command line tool for finding duplicate files. Recursively scans directories for duplicate files and generates a `JSON` file with the results.

## Arguments
|Name|Description|Required|Notes|
|----|-----------|--------|-----|
|`path`|Specifies the root directory for the search.|Yes||
|`output`|Specifies the output file name.|Yes|Output format is JSON.|
|`exclude`|An array of regular expressions to exclude from the search. If any of the regular expressions match a file or folder, it will be excluded.|No||
|`include`|An array of regular expressions to restrict the search. If all of the regular expressions do not match a file or folder, it will be excluded.|No|`exclude` will take precedence if specified with `include`.|
|`minsize`|A human-readable file size. Any files smaller than this size will be excluded from the search.|No||
|`maxsize`|A human-readable file size. Any files larger than this size will be excluded from the search.|No||

## Info
Files are compared first by size then by `SHA256` checksum.