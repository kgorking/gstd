import gs;
import gs.testing;

test string_basic = [] {
    string s("hello world");
    test::assert_eq(s.size(), 11Z, "size should be 11");
    test::assert_eq(s[0], 'h', "first char should be 'h'");
    test::assert_eq(s[10], 'd', "last char should be 'd'");
    test::assert(!s.empty(), "string should not be empty");

    string sub = s.substr(6, 5);
    test::assert_eq(sub.size(), 5Z, "substr size should be 5");
    test::assert_eq(sub, string("world"), "substr should be 'world'");

    string empty;
    test::assert(empty.empty(), "empty string should be empty");
    test::assert_eq(empty.size(), 0Z, "empty string size should be 0");

    test::assert_eq(s, string("hello world"), "s should equal 'hello world'");
    test::assert_eq(string("hello world"), s, "'hello world' should equal s");
    test::assert_eq(s, s, "s should equal itself");

    // Test substr of substr
    string subsub = sub.substr(1, 3);
    test::assert_eq(subsub, string("orl"), "subsub should be 'orl'");

    // Test assignment
    string assigned = "test";
    test::assert_eq(assigned.size(), 4Z, "assigned size should be 4");
    test::assert_eq(assigned, string("test"), "assigned should be 'test'");
};

test string_utf8 = [] {
    // Test UTF-8 string with multibyte characters
    string s("héllo wörld");
    // "héllo wörld": 11 characters
    test::assert_eq(s.count(), 11Z, "count should be 11");
    test::assert_eq(s.size_bytes(), 13Z, "size_bytes should be 13");
    test::assert_eq(s[0], 'h', "first char should be 'h'");
    test::assert_eq(s[1].count(), 1Z, "second char count should be 1");
    test::assert_eq(s[1].size_bytes(), 2Z, "second char bytes should be 2");
    test::assert(s[1] == "é", "second char should be 'é'");
    test::assert_eq(s[2], 'l', "third char should be 'l'");
    test::assert(!s.empty(), "string should not be empty");

    // Test substr with multibyte characters
    string sub = s.substr(5, 6);  // space, w, ö, r, l, d = 6 characters
    test::assert_eq(sub.count(), 6Z, "substr count should be 6");

    // Test substr result matches expected
    string expected_sub_str(" wörld");
    test::assert_eq(sub, expected_sub_str, "substr should be ' wörld'");

    // Test substr of substr - get "örl" (3 characters)
    string subsub = sub.substr(2, 3);  // ö, r, l = 3 characters
    test::assert_eq(subsub.count(), 3Z, "subsub count should be 3");

    string expected_subsub_str("örl");
    test::assert_eq(subsub, expected_subsub_str, "subsub should be 'örl'");

    // Test with emoji (1 character = 4 bytes): 🚀
    string emoji("🚀");
    test::assert_eq(emoji.count(), 1Z, "emoji count should be 1");
    test::assert_eq(emoji.size_bytes(), 4Z, "emoji bytes should be 4");
    string expected_emoji("🚀");
    test::assert_eq(emoji, expected_emoji, "emoji should match");

    // Test UTF-8 literal assignment
    string assigned = "🚀";
    test::assert_eq(assigned.count(), 1Z, "assigned emoji count should be 1");
    test::assert_eq(assigned, string("🚀"), "assigned emoji should match");
};

test string_literals = [] {
    string ascii_literal = "abcdef";
    test::assert_eq(ascii_literal.count(), 6Z, "ascii count should be 6");

    string const expected_emoji("🚀");

    string emoji_ordinal_literal = "🚀";
    test::assert_eq(emoji_ordinal_literal.count(), 1Z, "emoji count should be 1");
    test::assert_eq(emoji_ordinal_literal, expected_emoji, "emoji literal should match");

    // Test UTF-8 escape sequence
    string emoji_escape = "\U0001F680";  // Rocket emoji using Unicode escape
    test::assert_eq(emoji_escape.count(), 1Z, "escape emoji count should be 1");
    test::assert_eq(emoji_escape, expected_emoji, "escape emoji should match");

    test::assert_eq(expected_emoji, string("🚀"), "emoji comparison should work");
};

test string_remove_prefix = [] {
    // Test remove_prefix with ASCII
    string ascii("hello world");
    ascii.remove_prefix(6);
    test::assert_eq(ascii, string("world"), "after remove_prefix should be 'world'");
    test::assert_eq(ascii.size(), 5UZ, "size after remove_prefix should be 5");

    // Test remove_prefix with zero characters
    string ascii2("hello");
    ascii2.remove_prefix(0);
    test::assert_eq(ascii2, string("hello"), "remove_prefix(0) should not change");

    // Test remove_prefix with all characters
    string ascii3("hello");
    ascii3.remove_prefix(5);
    test::assert(ascii3.empty(), "remove_prefix all should empty string");
    test::assert_eq(ascii3.size(), 0Z, "empty size should be 0");

    // Test remove_prefix with UTF-8 multibyte characters
    string utf8_str("héllo wörld");
    utf8_str.remove_prefix(5);  // Remove "héllo"
    test::assert_eq(utf8_str.count(), 6Z, "utf8 after remove_prefix count should be 6");

    string expected_str(" wörld");
    test::assert_eq(utf8_str, expected_str, "utf8 after remove_prefix should match");

    // Test with 4-byte emoji character
    string rocket_emoji = "🚀🚀🚀";
    rocket_emoji.remove_prefix(2);  // Remove first two emojis
    test::assert_eq(rocket_emoji.count(), 1Z, "emoji count after remove_prefix should be 1");
    string expected_rocket("🚀");
    test::assert_eq(rocket_emoji, expected_rocket, "emoji after remove_prefix should match");
};

test string_remove_postfix = [] {
    // Test remove_postfix with ASCII
    string ascii("hello world");
    ascii.remove_postfix(6);
    test::assert_eq(ascii, string("hello"), "after remove_postfix should be 'hello'");
    test::assert_eq(ascii.count(), 5Z, "count after remove_postfix should be 5");

    // Test remove_postfix with zero characters
    string ascii2("hello");
    ascii2.remove_postfix(0);
    test::assert_eq(ascii2, string("hello"), "remove_postfix(0) should not change");

    // Test remove_postfix with all characters
    string ascii3("hello");
    ascii3.remove_postfix(5);
    test::assert(ascii3.empty(), "remove_postfix all should be empty");
    test::assert_eq(ascii3.count(), 0Z, "empty count should be 0");
    test::assert_eq(ascii3.size(), 0Z, "empty size should be 0");

    // Test remove_postfix with UTF-8 multibyte characters
    string utf8_str("héllo wörld");
    utf8_str.remove_postfix(6);  // Remove " wörld"
    test::assert_eq(utf8_str.count(), 5Z, "utf8 count after remove_postfix should be 5");

    string expected_str("héllo");
    test::assert_eq(utf8_str, expected_str, "utf8 after remove_postfix should match");

    // Test with 4-byte emoji character
    string rocket_emoji = "🚀🚀🚀";
    rocket_emoji.remove_postfix(2);  // Remove last two emojis
    test::assert_eq(rocket_emoji.count(), 1Z, "emoji count after remove_postfix should be 1");
    string expected_rocket("🚀");
    test::assert_eq(rocket_emoji, expected_rocket, "emoji after remove_postfix should match");
};

test string_format = [] {
    string text = "world";
    string formatted = string::fmt("Hello, {}!", text);
    test::assert_eq(formatted, string("Hello, world!"), "formatted string should match");

    // Test with UTF-8 characters
    string emoji = "🚀";
    string expected = "Emoji: 🚀";
    formatted = string::fmt("Emoji: {}", emoji);
    test::assert_eq(formatted, expected, "formatted emoji should match");
};
