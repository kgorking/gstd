import std;
import gs;

static task<int> dependency() {
	int const val = 100 + 31; // (std::rand() % 1000);
	//std::chrono::milliseconds d(val);
	//std::this_thread::sleep_for(d);
	//std::println("dependency finished after {0} ms", d.count());
	co_return val;
}

static task<int> dependent1() {
	int const val = co_await dependency();
	co_return val * 2;
}
static task<int> dependent2() {
	int const val = co_await dependency();
	co_return val * 3;
}

int main() {
	auto const [v1, v2] = wait_all(dependent1(), dependent2());
	std::println("Results: {0}, {1}", v1, v2);
	return 0;
}
