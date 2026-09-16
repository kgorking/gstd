import std;
import gs;

task<void> print_letters(char c) {
	for (int i = 0; i < 20; ++i) {
		std::putchar(c);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	co_return;
}

int dmain() {
	return 0;
}