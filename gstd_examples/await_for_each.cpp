import std;
import gs;

sequence<char> numbers() {
	co_yield '0';
	co_yield '3';
	co_yield '6';
}

int main() {
	// 012.345.678.
	numbers().for_each([](char c) {
		std::putchar(c);
		std::putchar(c + 1);
		std::putchar(c + 2);
		std::putchar('.');
		});
	std::putchar('\n');

	// 036147258...
	numbers().await_for_each([](char c) -> task<void> {
		std::putchar(c);
		co_await task_barrier;

		std::putchar(c + 1);
		co_await task_barrier;

		std::putchar(c + 2);
		co_await task_barrier;

		std::putchar('.');
		});
	std::putchar('\n');

	// 036147258...
	std::vector<char> vec_numbers{ std::from_range, numbers() };
	for (char c : vec_numbers) std::putchar(c);
	for (char c : vec_numbers) std::putchar(c + 1);
	for (char c : vec_numbers) std::putchar(c + 2);
	for (char c : vec_numbers) std::putchar('.' + (c-c));
	std::putchar('\n');
}
