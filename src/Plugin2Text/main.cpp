#include "esp_to_text.hpp"
#include "text_to_esp.hpp"
#include "common.hpp"
#include "esp_parser.hpp"
#include <stdio.h>
#include "os.hpp"

static void print_usage(const char* hint) {
    puts(hint);
    puts(
        "Usage: plugin2text.exe <source file> [destination file]\n"
        "\n"
        "    <source file>              file to convert (*.esp, *.esm, *.esl, *.txt)\n"
        "    [destination file]         output path\n"
        "\n"
        "Options:\n"
        "\n"
        "    --time                     output elapsed time in stdout\n"
        "\n"
        "Text serialization options:\n"
        "\n"
        "    --export-timestamp         write timestamps for records\n"
        "    --preserve-record-order    always write records in the same order as in ESP\n"
        "    --preserve-junk            do not clear fields that may contain junk data\n"
        "\n"
        "If <source file> has ESP/ESM/ESL file extension, then <source file> will be converted\n"
        "to text format. If <source file> has TXT extension, then <source file> will be converted\n"
        "to TES plugin.\n"
        "\n"
        "If [destination file] is omitted, then [destination file] is <source file> with extension\n"
        "changed to plugin or text format.\n"
        "\n"
        "Examples:\n"
        "    plugin2text.exe Skyrim.esm Skyrim.txt\n"
        "        convert Skyrim.esm to text format and write resulting file to Skyrim.txt\n"
        "\n"
        "    plugin2text.exe Dawnguard.txt Dawnguard.esm\n"
        "        convert Dawnguard.txt to TES plugin and write resulting file to Dawnguard.esm\n"
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

struct Args {
    const wchar_t* source_file = nullptr;
    const wchar_t* destination_file = nullptr;
    ProgramOptions options = ProgramOptions::None;
    bool time = false;

    void parse() {
        int argc = 0;
        const auto argv = get_command_line_args(&argc);

        for (int i = 1; i < argc; ++i) {
            auto arg = argv[i];
            if (string_starts_with(arg, L"--")) {
                arg += 2;
                if (string_equals(arg, L"export-timestamp") || string_equals(arg, L"export-timestamps")) {
                    options |= ProgramOptions::ExportTimestamp;
                } else if (string_equals(arg, L"time")) {
                    time = true;
                } else if (string_equals(arg, L"preserve-record-order")) {
                    options |= ProgramOptions::PreserveRecordOrder;
                } else if (string_equals(arg, L"preserve-junk")) {
                    options |= ProgramOptions::PreserveJunk;
                } else {
                    wprintf(L"warning: unknown option \"--%s\"\n", arg);
                }
            } else {
                if (!source_file) {
                    source_file = arg;
                } else if (!destination_file) {
                    destination_file = arg;
                } else {
                    wprintf(L"warning: unknown argument \"%s\"\n", arg);
                }
            }
        }
    }
};

int main() {
    Args args;
    args.parse();

    if (!args.source_file) {
        print_usage("<source file> argument is missing.\n");
        return 1;
    }

    const auto source_file = args.source_file;
    const auto source_file_extension = get_file_extension(source_file);
    const auto destination_file = args.destination_file ? args.destination_file : replace_destination_file_extension(source_file, source_file_extension);

    const auto start = args.time ? get_current_timestamp() : 0;

    if (string_equals(source_file_extension, L".txt")) {
        text_to_esp(source_file, destination_file);
    } else if (string_equals(source_file_extension, L".esp") || string_equals(source_file_extension, L".esm") || string_equals(source_file_extension, L".esl")) {
        EspParser parser;
        parser.init(args.options);
        defer(parser.dispose());
        
        const auto file = read_file(source_file);
        defer(delete[] file.data);

        parser.parse(file);

        esp_to_text(args.options, parser.model, destination_file);
    } else {
        exit_error(L"unrecognized source file extension \"%s\" (\"%s\")", source_file_extension, source_file);
    }

    if (args.time) {
        printf("Time elapsed: %f seconds\n", timestamp_to_seconds(start, get_current_timestamp()));
    }
    
    return 0;
}
