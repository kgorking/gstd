import gs;
import gs.testing;

auto simpleTest = [] -> test {
	test::assert(42 == 2 * 21);
	co_return;
}();
