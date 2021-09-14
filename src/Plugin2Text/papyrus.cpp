#include "papyrus.hpp"
#include "parseutils.hpp"
#include "os.hpp"
#include "array.hpp"
#include <stdio.h>
#include <stdlib.h>

constexpr String BeginAliasProperty = ";BEGIN ALIAS PROPERTY ";
constexpr String EndAliasProperty = ";END ALIAS PROPERTY";
constexpr String BeginFragment = ";BEGIN FRAGMENT ";
constexpr String EndFragment = ";END FRAGMENT";
constexpr String AliasPropertyType = ";ALIAS PROPERTY TYPE ";
constexpr String NextFragmentIndex = ";NEXT FRAGMENT INDEX ";
constexpr String BeginCode = ";BEGIN CODE";
constexpr String EndCode = ";END CODE";

static String string_between(String source, const String& begin, const String& end) {
    auto begin_index = source.index_of(begin);
    if (begin_index == -1) {
        return {};
    }

    source.advance(begin_index + begin.count);

    auto end_index = source.index_of(end);
    verify(end_index != -1);

    return { source.chars, end_index };
}

static String string_between_indices(const String& source, int begin, int end) {
    return { &source.chars[begin], end - begin };
}

static int find_and_advance(String& str, const String& substr) {
    auto index = str.index_of(substr);
    if (index != -1) {
        str.advance(index + substr.count);
    }
    return index;
}

static void skip_newlines(String& str) {
    while (str.count) {
        if (str.chars[0] == '\n' || str.chars[0] == '\r') {
            str.advance(1);
        } else {
            break;
        }
    }
}

static void expect(String& str, const String& substr) {
    verify(str.count >= substr.count);
    verify(memory_equals(str.chars, substr.chars, substr.count));
    str.advance(substr.count);
}

static String string_until_newline(String& source) {
    auto index = source.index_of("\r");
    if (index == -1) {
        index = source.index_of("\n");
        if (index == -1) {
            index = source.count;
        }
    }

    auto result = String{ source.chars, index };
    source.advance(index);
    return result;
}

struct AliasProperty {
    String name;
    String type;
    String text;

    void parse(String source) {
        name = string_until_newline(source);
        skip_newlines(source);
    
        expect(source, AliasPropertyType);
        type = string_until_newline(source);
        skip_newlines(source);

        text = source;
    }
};

struct Fragment {
    String name;
    String function_declaration;
    String code;
    String function_declaration_end;

    void parse(String source) {
        {
            auto index = source.index_of("\r");
            if (index == -1) {
                index = source.index_of("\n");
                verify(index != -1);
            }

            name = String{ &source.chars[0], index };
            source.advance(index);
            skip_newlines(source);
        }
        
        {
            const auto begin_index = source.index_of(BeginCode);
            verify(begin_index != -1);
            function_declaration = { &source.chars[0], begin_index };
            source.advance(begin_index + BeginCode.count);
        }

        {
            const auto end_index = source.index_of(EndCode);
            verify(end_index != -1);
            code = { source.chars, end_index };
            source.advance(end_index + EndCode.count);
        }

        function_declaration_end = source;
    }
};

struct FragmentCode {
    String scriptname;
    int next_fragment_index = 0;
    Array<AliasProperty> aliases{ tmpalloc };
    Array<Fragment> fragments{ tmpalloc };

    void parse(String source) {
        {
            skip_newlines(source);
            expect(source, NextFragmentIndex);
            int nread = 0;
            verify(1 == _snscanf_s(source.chars, source.count, "%d%n", &next_fragment_index, &nread));
            source.advance(nread);
        }

        {
            auto index = source.index_of(BeginAliasProperty);
            if (index == -1) {
                index = source.index_of(BeginFragment);
            }

            if (index != -1) {
                scriptname = { source.chars, index };
                source.advance(index);
            }
        }

        while (true) {
            const auto alias_text = string_between(source, BeginAliasProperty, EndAliasProperty);
            if (!alias_text.count) {
                break;
            }

            AliasProperty alias;
            alias.parse(alias_text);
            aliases.push(alias);

            source.advance(alias_text.count + BeginAliasProperty.count + EndAliasProperty.count);
            skip_newlines(source);
        }

        while (true) {
            const auto fragment_text = string_between(source, BeginFragment, EndFragment);
            if (!fragment_text.count) {
                break;
            }

            Fragment fragment;
            fragment.parse(fragment_text);
            fragments.push(fragment);

            source.advance(fragment_text.count + BeginFragment.count + EndFragment.count);
            skip_newlines(source);
        }

        qsort(aliases.data, aliases.count, sizeof(aliases.data[0]), [](void const* aa, void const* bb) -> int {
            auto a = (AliasProperty*)aa;
            auto b = (AliasProperty*)bb;
            return a->name.compare(b->name);
        });

        qsort(fragments.data, fragments.count, sizeof(fragments.data[0]), [](void const* aa, void const* bb) -> int {
            auto a = (Fragment*)aa;
            auto b = (Fragment*)bb;
            return a->name.compare(b->name);
        });
    }
};

String papyrus_sort_fragments(const String& source) {
    // @TODO @Test
    constexpr String BeginFragmentCode = ";BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment";
    constexpr String EndFragmentCode = ";END FRAGMENT CODE - Do not edit anything between this and the begin comment";

    const auto fragment_code = string_between(
        source,
        BeginFragmentCode,
        EndFragmentCode
    );

    if (fragment_code.count == 0) {
        return source;
    }

    FragmentCode code;
    code.parse(fragment_code);

    Slice output;
    output.start = tmpalloc.now;
    output.now = tmpalloc.now;
    output.end = tmpalloc.now + tmpalloc.remaining_size();

    output.write_string(BeginFragmentCode);
    output.write_literal("\r\n");
    if (code.next_fragment_index) {
        char index[50];
        int count = sprintf_s(index, "%d", code.next_fragment_index);
        verify(count > 0);

        output.write_string(NextFragmentIndex);
        output.write_string({ index, count });
        output.write_string(code.scriptname);
    }

    for (const auto& alias : code.aliases) {
        output.write_string(BeginAliasProperty);
        output.write_string(alias.name);
        output.write_literal("\r\n");

        output.write_string(AliasPropertyType);
        output.write_string(alias.type);
        output.write_literal("\r\n");

        output.write_string(alias.text);

        output.write_string(EndAliasProperty);
        output.write_literal("\r\n\r\n");
    }

    for (const auto& fragment : code.fragments) {
        output.write_string(BeginFragment);
        output.write_string(fragment.name);
        output.write_literal("\r\n");

        output.write_string(fragment.function_declaration);
        
        output.write_string(BeginCode);
        output.write_string(fragment.code);
        output.write_string(EndCode);

        output.write_string(fragment.function_declaration_end);
        output.write_string(EndFragment);
        output.write_literal("\r\n\r\n");
    }

    output.write_string(EndFragmentCode);
    auto remaining = source;
    remaining.advance(fragment_code.count + BeginFragmentCode.count + EndFragmentCode.count);
    output.write_string(remaining);

    ::operator new(output.size(), tmpalloc); // Advance tmpalloc.

    return { (char*)output.start, (int)output.size() };
}
