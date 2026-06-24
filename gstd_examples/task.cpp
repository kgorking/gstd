import std;
import gs;

task<void> print_letters(char c) {
	for (int i = 0; i < 20; ++i) {
		std::putchar(c);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	co_return;
}

int main() {
	auto tasks = std::views::iota('a', 'z')
		| std::views::transform(print_letters)
		| std::ranges::to<std::vector>();
	wait_all(tasks);
}