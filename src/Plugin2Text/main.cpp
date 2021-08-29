#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include "esp_to_text.hpp"
#include "text_to_esp.hpp"
#include "common.hpp"
#include "esp_parser.hpp"
#include <stdio.h>
#include "os.hpp"

static void print_usage(const char* hint) {
    printf("%s", hint);
    printf(
        "Usage: plugin2text.exe <source file> [destination file]\n"
        "\n"
        "       <source file>       file to convert (*.esp, *.esm, *.esl, *.txt)\n"
        "       [destination file]  output path\n"
        "\n"
        "If <source file> has ESP/ESM/ESL file extension, then <source file> will be converted\n"
        "to text format. If <source file> has TXT extension, then <source file> will be converted\n"
        "to TES plugin.\n"
        "\n"
        "If [destination file] is omitted, then [destination file] is <source file> with extension\n"
        "changed to plugin or text format.\n"
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
    int index = string_last_index_of(string, '.');
    return index ? &string[index] : L"";
}

static const wchar_t* replace_destination_file_extension(const wchar_t* source_file, const wchar_t* source_file_extension) {
    const wchar_t* destination_file_extension = L".txt";
    if (string_equals(source_file_extension, L".txt")) {
        destination_file_extension = L".esp";
    }

    const auto no_extension_count = wcslen(source_file) - wcslen(source_file_extension);
    const auto count = no_extension_count + wcslen(destination_file_extension);

    auto destination_file = new wchar_t[count + 1];
    memcpy(&destination_file[0], source_file, sizeof(wchar_t) * no_extension_count);
    memcpy(&destination_file[no_extension_count], destination_file_extension, sizeof(wchar_t) * wcslen(destination_file_extension));
    destination_file[count] = L'\0';
    return destination_file;
}

int main() {
    int argc = 0;
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    verify(argv);

    if (argc == 1) {
        print_usage("<source file> arguments are missing.\n\n");
        return 1;
    }

    const auto source_file = argv[1];
    const auto source_file_extension = get_file_extension(source_file);
    const auto destination_file = argc > 2 ? argv[2] : replace_destination_file_extension(source_file, source_file_extension);

    if (string_equals(source_file_extension, L".txt")) {
        text_to_esp(source_file, destination_file);
    } else if (string_equals(source_file_extension, L".esp") || string_equals(source_file_extension, L".esm") || string_equals(source_file_extension, L".esl")) {
        EspParser parser;
        parser.init();
        defer(parser.dispose());
        
        const auto file = read_file(source_file);
        defer(delete[] file.data);

        parser.parse(file);

        esp_to_text(parser.model, destination_file);
    } else {
        exit_error(L"unrecognized source file extension \"%s\" (\"%s\")", source_file_extension, source_file);
    }
    
    return 0;
}
