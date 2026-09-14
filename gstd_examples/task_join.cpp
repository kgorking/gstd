import std;
import gs;

int main() {
	auto yielder = [] -> task<int> {
		co_yield 1;
		co_yield 2;
		co_yield 3;
		co_return 1122;
		};

	auto tester = [&] -> task<int> {
		auto y = yielder();
		if (1 != co_await y) co_return 2;
		if (2 != co_await y) co_return 2;
		if (3 != co_await y) co_return 2;
		if (1122 != co_await y) co_return 2;
		co_return 1;
		};

	return tester().result();
}
