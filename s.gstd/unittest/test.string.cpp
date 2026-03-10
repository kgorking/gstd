#include "doctest.h"
import std;
import gs;

TEST_CASE("test.string") {
    string s("hello world");
    CHECK(s.size() == 11);
    CHECK(s[0] == 'h');
    CHECK(s[10] == 'd');
    CHECK(!s.empty());

    string sub = s.substr(6, 5);
    CHECK(sub.size() == 5);
    CHECK(sub == "world");

    string empty;
    CHECK(empty.empty());
    CHECK(empty.size() == 0);

    CHECK(s == "hello world");
    CHECK("hello world" == s);
    CHECK(s == s);

    // Test substr of substr
    string subsub = sub.substr(1, 3);
    CHECK(subsub == "orl");

    // Test assignment
    string assigned = "test";
    CHECK(assigned.size() == 4);
    CHECK(assigned == "test");
}

TEST_CASE("test.string_utf8") {
    // Test UTF-8 string with multibyte characters
    // Using explicit byte arrays to ensure proper UTF-8 encoding
    char8_t hello_world[] = { 
        u8'h', 0xc3, 0xa9,  // h + é (1 char = 2 bytes)
        u8'l', u8'l', u8'o', u8' ', u8'w', 
        0xc3, 0xb6,         // ö (1 char = 2 bytes)
        u8'r', u8'l', u8'd', 0 
    };
    string s(hello_world);
    // "héllo wörld": 11 characters (13 bytes)
    CHECK(s.size() == 11);
    CHECK(s[0] == u8'h');
    CHECK(s[1] == u8"é");
    CHECK(s[2] == u8'l');
    CHECK(!s.empty());

    // Test substr with multibyte characters
    // " wörld" starts at character position 5 (space), 6 characters long
    string sub = s.substr(5, 6);  // space, w, ö, r, l, d = 6 characters
    CHECK(sub.size() == 6);
    
    // Test substr result matches expected
    char8_t expected_sub[] = { u8' ', u8'w', 0xc3, 0xb6, u8'r', u8'l', u8'd', 0 };
    string expected_sub_str(expected_sub);
    CHECK(sub == expected_sub_str);

    // Test substr of substr - get "örl" (3 characters)
    // In " wörld", ö is at position 2, so substr(2, 3) gets ö, r, l
    string subsub = sub.substr(2, 3);  // ö, r, l = 3 characters
    CHECK(subsub.size() == 3);
    
    char8_t expected_subsub[] = { 0xc3, 0xb6, u8'r', u8'l', 0 };
    string expected_subsub_str(expected_subsub);
    CHECK(subsub == expected_subsub_str);

    // Test with emoji (1 character = 4 bytes): 🚀 = F0 9F 9A 80
    char8_t emoji_bytes[] = { 0xf0, 0x9f, 0x9a, 0x80, 0 };
    string emoji(emoji_bytes);
    CHECK(emoji.size() == 1);  // Rocket emoji is 1 character (4 bytes)
    string expected_emoji(emoji_bytes);
    CHECK(emoji == expected_emoji);

    // Test UTF-8 literal assignment
    char8_t rocket[] = { 0xf0, 0x9f, 0x9a, 0x80, 0 };
    string assigned = rocket;
    CHECK(assigned.size() == 1);
    string expected_assigned(rocket);
    CHECK(assigned == expected_assigned);
}

TEST_CASE("test.string_literals") {
    string ascii_literal = u8"abcdef";
    CHECK(ascii_literal.size() == 6);

    char8_t emoji_bytes[] = { 0xf0, 0x9f, 0x9a, 0x80, 0 };
    string const expected_emoji(emoji_bytes);

    string emoji_ordinal_literal = "🚀";
    CHECK(emoji_ordinal_literal.size() == 1);
    CHECK(emoji_ordinal_literal == expected_emoji);

    string emoji_utf8_literal = u8"🚀";
    CHECK(emoji_utf8_literal.size() == 1);
    CHECK(emoji_utf8_literal == expected_emoji);

    // Test UTF-8 escape sequence (should work regardless of source file encoding)
    string emoji_escape = u8"\U0001F680";  // Rocket emoji using Unicode escape
    CHECK(emoji_escape.size() == 1);
    CHECK(emoji_escape == expected_emoji);

    CHECK(expected_emoji == u8"🚀");
}