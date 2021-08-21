# plugin2text
Converts Skyrim SE *.esp to text format 

### Usage
```
Usage: plugin2text.exe <source file> <destination file>
        
        <source file>       file to convert (*.esp, *.esm, *.esl, *.txt)
        [destination file]  output path
        
If <source file> has ESP/ESM/ESL file extension, then <source file> will be converted
to text format. If <source file> has TXT extension, then <source file> will be converted
to TES plugin.
        
If [destination file] is omitted, then [destination file] is <source file> with extension
changed to plugin or text format.
        
Examples:
        plugin2text.exe Skyrim.esm Skyrim.txt
            convert Skyrim.esm to text format and write resulting file to Skyrim.txt
        
        plugin2text.exe Dawnguard.txt Dawnguard.esm
            convert Dawnguard.txt to TES plugin and write resulting file to Dawnguard.esm
```

### Example output
```
plugin2text version 1.00
---
TES4 [00000000] - File Header
  HEDR - Header
    Version
      1.700000
    Number Of Records
      0
    Next Object ID
      14462
  CNAM - Author
    "DEFAULT"
  MAST - Master File
    "Skyrim.esm"
  DATA
    0000000000000000
  MAST - Master File
    "Update.esm"
  DATA
    0000000000000000
  INTV
    01000000
```