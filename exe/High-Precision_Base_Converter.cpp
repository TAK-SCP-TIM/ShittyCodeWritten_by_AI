#include <iostream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace std;

// 大整数类，十进制表示，高位在前
class BigInt {
private:
	vector<int> digits; // 高位在索引0，低位在末尾
	
	// 去除前导零
	void trim() {
		while (digits.size() > 1 && digits.front() == 0) {
			digits.erase(digits.begin());
		}
	}
	
public:
	// 从字符串构造
	BigInt(const string& s = "0") {
		for (char c : s) {
			if (!isdigit(c)) throw invalid_argument("非数字字符");
			digits.push_back(c - '0');
		}
		if (digits.empty()) digits.push_back(0);
		trim();
	}
	
	// 从整数构造（仅用于0-15的小数字）
	BigInt(int n) {
		if (n < 0 || n > 15) throw invalid_argument("仅支持0-15的小整数");
		if (n == 0) digits.push_back(0);
		else {
			while (n > 0) {
				digits.insert(digits.begin(), n % 10);
				n /= 10;
			}
		}
	}
	
	bool is_zero() const {
		return digits.size() == 1 && digits[0] == 0;
	}
	
	string to_string() const {
		string res;
		for (int d : digits) res.push_back('0' + d);
		return res;
	}
	
	int to_int() const {
		if (digits.size() != 1) throw runtime_error("无法转换为小整数");
		return digits[0];
	}
	
	// 比较
	bool operator<(const BigInt& other) const {
		if (digits.size() != other.digits.size())
			return digits.size() < other.digits.size();
		for (size_t i = 0; i < digits.size(); ++i) {
			if (digits[i] != other.digits[i])
				return digits[i] < other.digits[i];
		}
		return false;
	}
	
	bool operator==(const BigInt& other) const {
		return digits == other.digits;
	}
	
	bool operator<=(const BigInt& other) const {
		return (*this < other) || (*this == other);
	}
	
	// 乘以小整数 (0-15)
	BigInt multiply_by_small(int x) const {
		if (x == 0) return BigInt(0);
		if (x == 1) return *this;
		vector<int> res(digits.size(), 0);
		int carry = 0;
		for (int i = digits.size() - 1; i >= 0; --i) {
			int prod = digits[i] * x + carry;
			res[i] = prod % 10;
			carry = prod / 10;
		}
		if (carry) res.insert(res.begin(), carry);
		BigInt result;
		result.digits = res;
		return result;
	}
	
	// 加小整数 (0-15)
	BigInt add_small(int x) const {
		if (x == 0) return *this;
		vector<int> res = digits;
		int carry = x;
		for (int i = res.size() - 1; i >= 0 && carry; --i) {
			int sum = res[i] + carry;
			res[i] = sum % 10;
			carry = sum / 10;
		}
		if (carry) res.insert(res.begin(), carry);
		BigInt result;
		result.digits = res;
		return result;
	}
	
	// 除以小整数，返回商和余数
	pair<BigInt, int> divide_by_small(int x) const {
		if (x == 0) throw invalid_argument("除数为0");
		vector<int> quot;
		int remainder = 0;
		for (int d : digits) {
			remainder = remainder * 10 + d;
			quot.push_back(remainder / x);
			remainder %= x;
		}
		BigInt quotient;
		quotient.digits = quot;
		quotient.trim();
		return {quotient, remainder};
	}
	
	// 减法（假设 *this >= other）
	BigInt subtract(const BigInt& other) const {
		vector<int> res = digits;
		int borrow = 0;
		int i = res.size() - 1, j = other.digits.size() - 1;
		while (i >= 0) {
			int sub = (j >= 0 ? other.digits[j] : 0) + borrow;
			if (res[i] < sub) {
				res[i] += 10 - sub;
				borrow = 1;
			} else {
				res[i] -= sub;
				borrow = 0;
			}
			--i; --j;
		}
		BigInt result;
		result.digits = res;
		result.trim();
		return result;
	}
	
	// 大整数除法，返回商和余数
	pair<BigInt, BigInt> divide(const BigInt& divisor) const {
		if (divisor.is_zero()) throw invalid_argument("除数为0");
		if (*this < divisor) return {BigInt(0), *this};
		
		vector<int> quot;
		BigInt remainder(0);
		
		for (int d : digits) {
			remainder = remainder.multiply_by_small(10).add_small(d);
			if (remainder < divisor) {
				if (!quot.empty()) quot.push_back(0);
				continue;
			}
			// 试商，从9到1
			int q = 0;
			for (int trial = 9; trial >= 1; --trial) {
				BigInt prod = divisor.multiply_by_small(trial);
				if (prod <= remainder) {
					q = trial;
					remainder = remainder.subtract(prod);
					break;
				}
			}
			quot.push_back(q);
		}
		
		BigInt quotient;
		quotient.digits = quot.empty() ? vector<int>{0} : quot;
		quotient.trim();
		return {quotient, remainder};
	}
	
	// 幂运算 base^exp，base为int，exp为int
	static BigInt power(int base, int exp) {
		BigInt result(1);
		for (int i = 0; i < exp; ++i) {
			result = result.multiply_by_small(base);
		}
		return result;
	}
};

// 字符转数值（0-15）
int char_to_digit(char c) {
	if (isdigit(c)) return c - '0';
	if (isalpha(c)) {
		c = toupper(c);
		if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
	}
	throw invalid_argument("非法字符");
}

// 数值转字符（0-15）
char digit_to_char(int d) {
	if (d < 10) return '0' + d;
	return 'A' + (d - 10);
}

// 将源进制字符串转换为十进制大整数
BigInt src_base_to_decimal_int(const string& s, int base) {
	BigInt result("0");
	for (char c : s) {
		int val = char_to_digit(c);
		result = result.multiply_by_small(base).add_small(val);
	}
	return result;
}

// 将十进制大整数转换为目标进制字符串
string decimal_int_to_dest_base(const BigInt& num, int base) {
	if (num.is_zero()) return "0";
	BigInt n = num;
	string result;
	while (!n.is_zero()) {
		auto [quot, rem] = n.divide_by_small(base);
		result.push_back(digit_to_char(rem));
		n = quot;
	}
	reverse(result.begin(), result.end());
	return result;
}

// 检查输入数字字符串的合法性
bool validate_number(const string& num_str, int base) {
	bool dot_seen = false;
	for (size_t i = 0; i < num_str.size(); ++i) {
		char c = num_str[i];
		if (c == '.') {
			if (dot_seen) return false; // 多个小数点
			dot_seen = true;
			continue;
		}
		try {
			int val = char_to_digit(c);
			if (val >= base) return false;
		} catch (...) {
			return false;
		}
	}
	// 不能全为空或只有小数点
	if (num_str.empty() || (dot_seen && num_str.size() == 1)) return false;
	return true;
}

// 分离整数部分和小数部分
pair<string, string> split_number(const string& num_str) {
	size_t dot_pos = num_str.find('.');
	if (dot_pos == string::npos) {
		return {num_str, ""};
	}
	string int_part = num_str.substr(0, dot_pos);
	string frac_part = num_str.substr(dot_pos + 1);
	return {int_part, frac_part};
}

int main() {
	cout << "高精度进制转换器（2-16进制，整数最多100位，小数最多100位）\n\n";
	
	int src_base, dest_base;
	string input_num;
	
	// 输入源进制
	while (true) {
		cout << "请输入源进制（2-16）: ";
		cin >> src_base;
		if (cin.fail() || src_base < 2 || src_base > 16) {
			cin.clear();
			cin.ignore(10000, '\n');
			cout << "输入错误，请重新输入！\n";
		} else {
			break;
		}
	}
	
	// 输入目标进制
	while (true) {
		cout << "请输入目标进制（2-16）: ";
		cin >> dest_base;
		if (cin.fail() || dest_base < 2 || dest_base > 16) {
			cin.clear();
			cin.ignore(10000, '\n');
			cout << "输入错误，请重新输入！\n";
		} else {
			break;
		}
	}
	
	// 输入数字
	cin.ignore(10000, '\n'); // 清除换行
	while (true) {
		cout << "请输入数字（允许小数点，整数部分最多100位，小数部分最多100位）: ";
		getline(cin, input_num);
		if (input_num.empty()) {
			cout << "输入不能为空！\n";
			continue;
		}
		if (!validate_number(input_num, src_base)) {
			cout << "数字中包含非法字符或超出进制范围，请重新输入！\n";
			continue;
		}
		// 检查长度限制
		auto [int_part, frac_part] = split_number(input_num);
		if (int_part.size() > 100) {
			cout << "整数部分不能超过100位！\n";
			continue;
		}
		if (frac_part.size() > 100) {
			cout << "小数部分不能超过100位！\n";
			continue;
		}
		break;
	}
	
	// 分离整数和小数部分
	auto [int_str, frac_str] = split_number(input_num);
	
	// 处理整数部分（如果为空，则为"0"）
	if (int_str.empty()) int_str = "0";
	
	// 转换整数部分
	BigInt decimal_int = src_base_to_decimal_int(int_str, src_base);
	string dest_int = decimal_int_to_dest_base(decimal_int, dest_base);
	
	// 转换小数部分
	string dest_frac;
	if (!frac_str.empty()) {
		// 计算分子 = 小数部分字符串转十进制大整数
		BigInt numerator = src_base_to_decimal_int(frac_str, src_base);
		// 分母 = src_base^len
		int frac_len = frac_str.size();
		BigInt denominator = BigInt::power(src_base, frac_len);
		
		// 最多生成100位小数
		for (int i = 0; i < 100; ++i) {
			numerator = numerator.multiply_by_small(dest_base);
			auto [quot, rem] = numerator.divide(denominator);
			// quot 应该是一位数（0-15）
			dest_frac.push_back(digit_to_char(quot.to_int()));
			numerator = rem;
			if (numerator.is_zero()) break;
		}
		
		// 去除小数部分末尾的零
		while (!dest_frac.empty() && dest_frac.back() == '0') {
			dest_frac.pop_back();
		}
	}
	
	// 组装结果
	string result = dest_int;
	if (!dest_frac.empty()) {
		result += "." + dest_frac;
	}
	
	// 输出结果
	cout << src_base << "进制转换为" << dest_base << "进制结果为: " << result << endl;
	
	return 0;
}
