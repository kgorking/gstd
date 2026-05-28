import gs;
import gs.testing;

test simpleTest2 = [] {
	test::is_true(42 == 21, "42 should equal 21");
	test::equals(42,  21);
	test::equals<21>(42);
};
