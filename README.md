# plugin2text
Converts Skyrim SE *.esp to text format 

### Usage
```
Usage: plugin2text.exe <source file> [destination file]
        
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

### Details
Conversion to text format is not lossless, which means that if you convert ESP to text format and then again into ESP you will not get original ESP file.
Following information is lost or changed when converting ESP to text:

* Record and group timestamp and version control info are overwritten with zeros.
* Some fields that may contain junk data are cleared with zeros (`XCLC` field flags in `CELL` record for example).
* Some fields are assumed to be constant, aka they always have same size and data inside them. These fields are not serialized to text format (some parts of `TRDT` field in `INFO` record for example).
* Records inside persistent/temporary cell children groups are sorted by Form ID for consistency (Creation Kit saves this information in random order).
* Floating point values may lose precision.

Even if none of these things apply to ESP file, if ESP constains compressed records, Plugin2Text may generate different compressed data for that record, 
which in turn may change record size/byte sequence, thus producing ESP that is not same as original ESP. Creation Kit uses really old zlib version and
I don't know what options CK uses to compress data.

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
  MAST - Master File
    "Update.esm"
  INTV - Tagified Strings
    1
```