import gs;
import std;

static task<int> sleep_task(int ms) {
	co_await std::suspend_always{};
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	co_return ms;
}

int main() {
	//int const duration = 100;
	auto tasks = std::array{
		sleep_task(1 /* duration*/),
		sleep_task(2 /* duration*/),
		sleep_task(3 /* duration*/),
		sleep_task(4 /* duration*/),
		sleep_task(5 /* duration*/)
	};

	for (int result : yield_all(tasks))
		std::println("yield: {}", result);
}
