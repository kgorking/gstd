#include "doctest.h"
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
    string s("héllo wörld");
    // "héllo wörld": 11 characters
    REQUIRE(s.count() == 11);
    REQUIRE(s.size_bytes() == 13);  // héllo wörld in UTF-8 is 13 bytes
    CHECK(s[0] == 'h');
    CHECK(s[1].count() == 1);
    CHECK(s[1].size_bytes() == 2);
    CHECK(2 == std::strlen("é"));
    bool x = s[1] == "é";
    CHECK(x);
    CHECK(s[2] == 'l');
    CHECK(!s.empty());

    // Test substr with multibyte characters
    // " wörld" starts at character position 5 (space), 6 characters long
    string sub = s.substr(5, 6);  // space, w, ö, r, l, d = 6 characters
    CHECK(sub.count() == 6);
    
    // Test substr result matches expected
    string expected_sub_str(" wörld");
    CHECK(sub == expected_sub_str);

    // Test substr of substr - get "örl" (3 characters)
    // In " wörld", ö is at position 2, so substr(2, 3) gets ö, r, l
    string subsub = sub.substr(2, 3);  // ö, r, l = 3 characters
    CHECK(subsub.count() == 3);
    
    string expected_subsub_str("örl");
    CHECK(subsub == expected_subsub_str);

    // Test with emoji (1 character = 4 bytes): 🚀
    string emoji("🚀");
    CHECK(emoji.count() == 1);  // Rocket emoji is 1 character
    CHECK(emoji.size_bytes() == 4);  // But 4 bytes
    string expected_emoji("🚀");
    CHECK(emoji == expected_emoji);

    // Test UTF-8 literal assignment
    string assigned = "🚀";
    CHECK(assigned.count() == 1);
    CHECK(4 == std::strlen("🚀"));
    CHECK(assigned == "🚀");
}

TEST_CASE("test.string_literals") {
    string ascii_literal = "abcdef";
    CHECK(ascii_literal.count() == 6);

    string const expected_emoji("🚀");

    string emoji_ordinal_literal = "🚀";
    CHECK(emoji_ordinal_literal.count() == 1);
    CHECK(emoji_ordinal_literal == expected_emoji);

    // Test UTF-8 escape sequence (should work regardless of source file encoding)
    string emoji_escape = "\U0001F680";  // Rocket emoji using Unicode escape
    CHECK(emoji_escape.count() == 1);
    CHECK(emoji_escape == expected_emoji);

    CHECK(expected_emoji == "🚀");
}

TEST_CASE("test.string_remove_prefix") {
    // Test remove_prefix with ASCII
    string ascii("hello world");
    ascii.remove_prefix(6);
    CHECK(ascii == "world");
    CHECK(ascii.size() == 5);

    // Test remove_prefix with zero characters
    string ascii2("hello");
    ascii2.remove_prefix(0);
    CHECK(ascii2 == "hello");

    // Test remove_prefix with all characters
    string ascii3("hello");
    ascii3.remove_prefix(5);
    CHECK(ascii3.empty());
    CHECK(ascii3.size() == 0);

    // Test remove_prefix with UTF-8 multibyte characters
    string utf8_str("héllo wörld");
    // "héllo wörld": 11 characters
    utf8_str.remove_prefix(5);  // Remove "héllo"
    CHECK(utf8_str.count() == 6);  // Should be " wörld"
    
    string expected_str(" wörld");
    CHECK(utf8_str == expected_str);

    // Test with 4-byte emoji character
    string rocket_emoji = "🚀🚀🚀";
    rocket_emoji.remove_prefix(2);  // Remove first two emojis
    CHECK(rocket_emoji.count() == 1);
    string expected_rocket("🚀");
    CHECK(rocket_emoji == expected_rocket);
}

TEST_CASE("test.string_remove_postfix") {
    // Test remove_postfix with ASCII
    string ascii("hello world");
    ascii.remove_postfix(6);
    CHECK(ascii == "hello");
    CHECK(ascii.count() == 5);

    // Test remove_postfix with zero characters
    string ascii2("hello");
    ascii2.remove_postfix(0);
    CHECK(ascii2 == "hello");

    // Test remove_postfix with all characters
    string ascii3("hello");
    ascii3.remove_postfix(5);
    CHECK(ascii3.empty());
    CHECK(ascii3.count() == 0);
    CHECK(ascii3.size() == 0);

    // Test remove_postfix with UTF-8 multibyte characters
    string utf8_str("héllo wörld");
    // "héllo wörld": 11 characters
    utf8_str.remove_postfix(6);  // Remove " wörld"
    CHECK(utf8_str.count() == 5);  // Should be "héllo"
    
    string expected_str("héllo");
    CHECK(utf8_str == expected_str);

    // Test with 4-byte emoji character
    string rocket_emoji = "🚀🚀🚀";
    rocket_emoji.remove_postfix(2);  // Remove last two emojis
    CHECK(rocket_emoji.count() == 1);
    string expected_rocket("🚀");
    CHECK(rocket_emoji == expected_rocket);
}
TEST_CASE("test.string.format") {
    string text = "world";
    string formatted = string::fmt("Hello, {}!", text);
    CHECK(formatted == "Hello, world!");

    // Test with UTF-8 characters
    string emoji = "🚀";
    string expected = "Emoji: 🚀";
    formatted = string::fmt("Emoji: {}", emoji);
    CHECK(formatted == expected);
}
