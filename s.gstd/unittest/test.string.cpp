import gs;

test string_basic = [] {
    string s("hello world");
    test::equals(s.size(), 11Z, "size should be 11");
	test::equals(s[0], 'h', "first char should be 'h'");
    test::equals(s[10], 'd', "last char should be 'd'");
    test::is_true(!s.empty(), "string should not be empty");

    string sub = s.substr(6, 5);
    test::equals<5Z>(sub.size(), "substr size should be 5");
    test::equals(sub, "world", "substr should be 'world'");

    string empty;
    test::is_true(empty.empty(), "empty string should be empty");
    test::equals(empty.size(), 0Z, "empty string size should be 0");

    test::equals(s, "hello world", "s should equal 'hello world'");
    test::equals("hello world", s, "'hello world' should equal s");
    test::equals(s, s, "s should equal itself");

    // Test substr of substr
    string subsub = sub.substr(1, 3);
    test::equals(subsub, "orl", "subsub should be 'orl'");

    // Test assignment
    string assigned = "test";
    test::equals(assigned.size(), 4Z, "assigned size should be 4");
    test::equals(assigned, "test", "assigned should be 'test'");
};

test string_utf8 = [] {
    // Test UTF-8 string with multibyte characters
    string s("héllo wörld");
    // "héllo wörld": 11 characters
    test::equals(s.count(), 11Z, "count should be 11");
    test::equals(s.size_bytes(), 13Z, "size_bytes should be 13");
    test::equals(s[0], 'h', "first char should be 'h'");
    test::equals(s[1] & 0xC0, 0x80, "second char bytes should be utf-8 continuation bytes");
    test::is_true(s[1] == 'é', "second char should be 'é'");
    test::equals(s[2], 'l', "third char should be 'l'");
    test::is_true(!s.empty(), "string should not be empty");

    // Test substr with multibyte characters
    string sub = s.substr(5, 6);  // space, w, ö, r, l, d = 6 characters
    test::equals(sub.count(), 6Z, "substr count should be 6");

    // Test substr result matches expected
    string expected_sub_str(" wörld");
    test::equals(sub, expected_sub_str, "substr should be ' wörld'");

    // Test substr of substr - get "örl" (3 characters)
    string subsub = sub.substr(2, 3);  // ö, r, l = 3 characters
    test::equals(subsub.count(), 3Z, "subsub count should be 3");

    string expected_subsub_str("örl");
    test::equals(subsub, expected_subsub_str, "subsub should be 'örl'");

    // Test with emoji (1 character = 4 bytes): 🚀
    string emoji("🚀");
    test::equals(emoji.count(), 1Z, "emoji count should be 1");
    test::equals(emoji.size_bytes(), 4Z, "emoji bytes should be 4");
    test::equals(emoji[0], '🚀', "emoji should be '🚀'");
    string expected_emoji("🚀");
    test::equals(emoji, expected_emoji, "emoji should match");

    // Test UTF-8 literal assignment
    string assigned = "🚀";
    test::equals(assigned.count(), 1Z, "assigned emoji count should be 1");
    test::equals(assigned, string("🚀"), "assigned emoji should match");
};

test string_literals = [] {
    string ascii_literal = "abcdef";
    test::equals(ascii_literal.count(), 6Z, "ascii count should be 6");

    string const expected_emoji("🚀");

    string emoji_ordinal_literal = "🚀";
    test::equals(emoji_ordinal_literal.count(), 1Z, "emoji count should be 1");
    test::equals(emoji_ordinal_literal, expected_emoji, "emoji literal should match");

    // Test UTF-8 escape sequence
    string emoji_escape = "\U0001F680";  // Rocket emoji using Unicode escape
    test::equals(emoji_escape.count(), 1Z, "escape emoji count should be 1");
    test::equals(emoji_escape, expected_emoji, "escape emoji should match");

    test::equals(expected_emoji, string("🚀"), "emoji comparison should work");
};

test string_remove_prefix = [] {
    // Test remove_prefix with ASCII
    string ascii("hello world");
    ascii.remove_prefix(6);
    test::equals(ascii, string("world"), "after remove_prefix should be 'world'");
    test::equals(ascii.size(), 5UZ, "size after remove_prefix should be 5");

    // Test remove_prefix with zero characters
    string ascii2("hello");
    ascii2.remove_prefix(0);
    test::equals(ascii2, string("hello"), "remove_prefix(0) should not change");

    // Test remove_prefix with all characters
    string ascii3("hello");
    ascii3.remove_prefix(5);
    test::is_true(ascii3.empty(), "remove_prefix all should empty string");
    test::equals(ascii3.size(), 0Z, "empty size should be 0");

    // Test remove_prefix with UTF-8 multibyte characters
    string utf8_str("héllo wörld");
    utf8_str.remove_prefix(5);  // Remove "héllo"
    test::equals(utf8_str.count(), 6Z, "utf8 after remove_prefix count should be 6");

    string expected_str(" wörld");
    test::equals(utf8_str, expected_str, "utf8 after remove_prefix should match");

    // Test with 4-byte emoji character
    string rocket_emoji = "🚀🚀🚀";
    rocket_emoji.remove_prefix(2);  // Remove first two emojis
    test::equals(rocket_emoji.count(), 1Z, "emoji count after remove_prefix should be 1");
    string expected_rocket("🚀");
    test::equals(rocket_emoji, expected_rocket, "emoji after remove_prefix should match");
};

test string_remove_postfix = [] {
    // Test remove_postfix with ASCII
    string ascii("hello world");
    ascii.remove_postfix(6);
    test::equals(ascii, string("hello"), "after remove_postfix should be 'hello'");
    test::equals(ascii.count(), 5Z, "count after remove_postfix should be 5");

    // Test remove_postfix with zero characters
    string ascii2("hello");
    ascii2.remove_postfix(0);
    test::equals(ascii2, string("hello"), "remove_postfix(0) should not change");

    // Test remove_postfix with all characters
    string ascii3("hello");
    ascii3.remove_postfix(5);
    test::is_true(ascii3.empty(), "remove_postfix all should be empty");
    test::equals(ascii3.count(), 0Z, "empty count should be 0");
    test::equals(ascii3.size(), 0Z, "empty size should be 0");

    // Test remove_postfix with UTF-8 multibyte characters
    string utf8_str("héllo wörld");
    utf8_str.remove_postfix(6);  // Remove " wörld"
    test::equals(utf8_str.count(), 5Z, "utf8 count after remove_postfix should be 5");

    string expected_str("héllo");
    test::equals(utf8_str, expected_str, "utf8 after remove_postfix should match");

    // Test with 4-byte emoji character
    string rocket_emoji = "🚀🚀🚀";
    rocket_emoji.remove_postfix(2);  // Remove last two emojis
    test::equals(rocket_emoji.count(), 1Z, "emoji count after remove_postfix should be 1");
    string expected_rocket("🚀");
    test::equals(rocket_emoji, expected_rocket, "emoji after remove_postfix should match");
};

test string_format = [] {
    string text = "world";
    string formatted = fmt("Hello, {}!", text);
    test::equals(formatted, string("Hello, world!"), "formatted string should match");

    // Test with UTF-8 characters
    string emoji = "🚀";
    string expected = "Emoji: 🚀";
    formatted = fmt("Emoji: {}", emoji);
    test::equals(formatted, expected, "formatted emoji should match");
};
