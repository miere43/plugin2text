# plugin2text
Converts Skyrim SE *.esp to text format 

## Usage
```
Usage: plugin2text.exe <source file> <destination file>
        
        <source file>       file to convert (*.esp, *.esm, *.esl, *.txt)
        <destination file>  output path
        
If <source file> has ESP/ESM/ESL file extension, then <source file> will be converted
to text format. If <source file> has TXT extension, then <source file> will be converted
to TES plugin.
        
Examples:
        plugin2text.exe Skyrim.esm Skyrim.txt
            convert Skyrim.esm to text format and write resulting file to Skyrim.txt
        
        plugin2text.exe Dawnguard.txt Dawnguard.esm
            convert Dawnguard.text to TES plugin and write resulting file to Dawnguard.esm
```
