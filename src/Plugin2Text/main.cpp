#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include "esp_to_text.hpp"
#include "text_to_esp.hpp"
#include "common.hpp"
#include <stdio.h>

static void print_usage(const char* hint) {
    printf("%s", hint);
    printf(
        "Usage: plugin2text.exe <source file> <destination file>\n"
        "\n"
        "       <source file>       file to convert (*.esp, *.esm, *.esl, *.txt)\n"
        "       <destination file>  output path\n"
        "\n"
        "If <source file> has ESP/ESM/ESL file extension, then <source file> will be converted\n"
        "to text format. If <source file> has TXT extension, then <source file> will be converted\n"
        "to TES plugin.\n"
        "\n"
        "Examples:\n"
        "       plugin2text.exe Skyrim.esm Skyrim.txt\n"
        "           convert Skyrim.esm to text format and write resulting file to Skyrim.txt\n"
        "\n"
        "       plugin2text.exe Dawnguard.txt Dawnguard.esm\n"
        "           convert Dawnguard.txt to TES plugin and write resulting file to Dawnguard.esm\n"
    );
}

static const wchar_t* get_file_extension(const wchar_t* string) {
    const auto count = wcslen(string);
    for (int i = count - 1; i >= 0; --i) {
        if (string[i] == '.') {
            return &string[i];
        }
    }
    return L"";
}

int main() {
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    verify(argv);

    if (argc == 1) {
        print_usage("<source file> and <destination file> arguments are missing.\n\n");
        return 1;
    } else if (argc == 2) {
        print_usage("<destination file> argument is missing.\n\n");
        return 1;
    }

    const auto source_file = argv[1];
    const auto destination_file = argv[2];

    auto source_file_extension = get_file_extension(source_file);
    if (0 == wcscmp(source_file_extension, L".txt")) {
        text_to_esp(source_file, destination_file);
    } else if (0 == wcscmp(source_file_extension, L".esp") || 0 == wcscmp(source_file_extension, L".esm") || 0 == wcscmp(source_file_extension, L".esl")) {
        esp_to_text(source_file, destination_file);
    } else {
        exit_error(L"unrecognized source file extension \"%s\" (\"%s\")", source_file_extension, source_file);
    }
    
    return 0;
}
