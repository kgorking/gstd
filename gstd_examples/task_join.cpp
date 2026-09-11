import std;
import gs;

static task<int> dependency() {
	co_return 31;
}

static task<int> d;

static task<int> dependent1() {
	std::println("in thread {}", std::this_thread::get_id());
	co_await thread_pool::instance();
	std::println("now in thread {}", std::this_thread::get_id());
	int const val = co_await d;
	co_return val * 2;
}
static task<int> dependent2() {
	int const val = co_await d;
	co_return val * 3;
}

int main() {
	int loops = 10;
	while (loops--) {
		d = dependency();
		auto const [v1, v2] = wait_all(dependent1(), dependent2());
		std::println("loop {}: {}, {}", loops, v1, v2);
	}
	return 0;
}
