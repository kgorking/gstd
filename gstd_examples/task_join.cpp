import std;
import gs;

static task<void> test(channel<int>& ch, int r) {
	co_await thread_pool::switch_to_thread();
	int result = 100 + std::rand() % 1024;
	std::this_thread::sleep_for(std::chrono::milliseconds(result));
	ch << r;
}

int main() {
	channel<int> ch;
	auto t1 = test(ch, 1);
	auto t2 = test(ch, 2);
	auto t3 = test(ch, 3);
	return ch.get() + ch.get() + ch.get();
}
