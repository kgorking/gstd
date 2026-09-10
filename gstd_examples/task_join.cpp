import std;
import gs;

static task<int> dependency() {
	co_return 100 + 31;
}

static task<int> d;

static task<int> dependent1() {
	int const val = co_await d;
	co_return val * 2;
}
static task<int> dependent2() {
	int const val = co_await d;
	co_return val * 3;
}

int main() {
	int loops = 100;
	while (loops--) {
		d = dependency();
		auto const [v1, v2] = wait_all(dependent1(), dependent2());
		std::println("Results {}: {}, {}", loops, v1, v2);
	}
	return 0;
}
