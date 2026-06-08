import gs.testing;
import std;

int main() {
	char32_t c = '🚀';
	char32_t c1 = 'c';
	std::bitset<32> bc(c);
	if (c1 == 'c') {
		std::println("yo");
	}
	gs::testing::run_all_tests();
	return 0;
}
