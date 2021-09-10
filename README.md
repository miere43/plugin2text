# plugin2text ![Build status](https://github.com/miere43/plugin2text/actions/workflows/msbuild.yml/badge.svg)
Converts Skyrim SE *.esp to text format 

### Usage
```
Usage: plugin2text.exe <source file> [destination file]

    <source file>              file to convert (*.esp, *.esm, *.esl, *.txt)
    [destination file]         output path

Options:

    --time                     output elapsed time in stdout

Text serialization options:

    --export-timestamp         write timestamps for records
    --preserve-order           always write records/fields in the same order as in ESP
    --preserve-junk            do not clear fields that may contain junk data
    --export-related-files     export files required for mod to function (scripts,
                               facegen textures, SEQ file). --data-folder and
                               --export-folder options must be set

Export options (when using --export-related-files):

    --data-folder              path to Skyrim SE Data folder, by default tries to
                               find it from installation path
    --export-folder            path to folder where export files will be written

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
      1.7
    Number Of Records
      0
    Next Object ID
      [0000387E]
  CNAM - Author
    "DEFAULT"
  MAST - Master File
    "Skyrim.esm"
  MAST - Master File
    "Update.esm"
  INTV - Tagified Strings
    1
```

### Building
Look at `.github/workflows/msbuild.yml` for reference.

#### Dependencies
* Microsoft Visual Studio 2022 Preview, with following components:
    * Desktop environment for C++
    * C++ CMake tools for Windows

#### Build steps
1. Build and install `src/zlib-1.2.11` CMake project with Visual Studio
2. Build `src/Plugin2Text/Plugin2Text.sln` with Visual Studio

### Credits
* [zlib-ng](https://github.com/zlib-ng/zlib-ng) 
* [base64 by René Nyffenegger](https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp)
* [UESP](https://en.uesp.net/wiki/Skyrim_Mod:File_Formats) - file format info
* [xEdit](https://github.com/TES5Edit/TES5Edit) - used for validation

### License
MIT