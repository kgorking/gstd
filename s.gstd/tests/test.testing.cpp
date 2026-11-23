import gs;

// Will fail at compile time if the test fails.
test compiletimeTest = [] {
	test::equals(42, 21 * 2);
	};

// Will fail at runtime if the test fails.
test runtimeTest = [] {
	test::equals(42, 21 * 2);
	};
