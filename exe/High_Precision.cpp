#include <iostream>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <cctype>
#include <regex>
#include <stdexcept>

using namespace std;

// 大整数类（无符号，绝对值）
class BigInt {
private:
	string digits; // 数字字符串，无前导零，但可以是 "0"
	
	// 辅助函数：移除前导零
	void removeLeadingZeros() {
		size_t pos = digits.find_first_not_of('0');
		if (pos == string::npos) {
			digits = "0";
		} else {
			digits = digits.substr(pos);
		}
	}
	
public:
	// 默认构造为0
	BigInt() : digits("0") {}
	
	// 从字符串构造，假设字符串只包含数字（无符号）
	explicit BigInt(const string& s) : digits(s) {
		removeLeadingZeros();
	}
	
	// 返回字符串形式
	string toString() const { return digits; }
	
	// 比较两个大整数（绝对值）
	bool operator==(const BigInt& other) const { return digits == other.digits; }
	bool operator!=(const BigInt& other) const { return digits != other.digits; }
	bool operator<(const BigInt& other) const {
		if (digits.length() != other.digits.length())
			return digits.length() < other.digits.length();
		return digits < other.digits;
	}
	bool operator<=(const BigInt& other) const { return !(other < *this); }
	bool operator>(const BigInt& other) const { return other < *this; }
	bool operator>=(const BigInt& other) const { return !(*this < other); }
	
	// 加法
	BigInt add(const BigInt& other) const {
		string a = digits;
		string b = other.digits;
		// 补零对齐
		int lenA = a.length(), lenB = b.length();
		if (lenA < lenB) {
			a = string(lenB - lenA, '0') + a;
		} else {
			b = string(lenA - lenB, '0') + b;
		}
		int carry = 0;
		string result;
		for (int i = a.length() - 1; i >= 0; --i) {
			int sum = (a[i] - '0') + (b[i] - '0') + carry;
			result.push_back(char(sum % 10 + '0'));
			carry = sum / 10;
		}
		if (carry) result.push_back(char(carry + '0'));
		reverse(result.begin(), result.end());
		return BigInt(result);
	}
	
	// 减法，要求 *this >= other
	BigInt sub(const BigInt& other) const {
		string a = digits;
		string b = other.digits;
		// 补零对齐
		int lenA = a.length(), lenB = b.length();
		if (lenA < lenB) {
			a = string(lenB - lenA, '0') + a;
		} else {
			b = string(lenA - lenB, '0') + b;
		}
		int borrow = 0;
		string result;
		for (int i = a.length() - 1; i >= 0; --i) {
			int diff = (a[i] - '0') - (b[i] - '0') - borrow;
			if (diff < 0) {
				diff += 10;
				borrow = 1;
			} else {
				borrow = 0;
			}
			result.push_back(char(diff + '0'));
		}
		reverse(result.begin(), result.end());
		return BigInt(result);
	}
	
	// 乘法
	BigInt mul(const BigInt& other) const {
		string a = digits;
		string b = other.digits;
		if (a == "0" || b == "0") return BigInt("0");
		int lenA = a.length(), lenB = b.length();
		vector<int> result(lenA + lenB, 0);
		for (int i = lenA - 1; i >= 0; --i) {
			for (int j = lenB - 1; j >= 0; --j) {
				int mul = (a[i] - '0') * (b[j] - '0');
				int sum = mul + result[i + j + 1];
				result[i + j + 1] = sum % 10;
				result[i + j] += sum / 10;
			}
		}
		string res;
		for (int num : result) {
			if (!(res.empty() && num == 0))
				res.push_back(char(num + '0'));
		}
		if (res.empty()) res = "0";
		return BigInt(res);
	}
	
	// 除法，返回商和余数，要求 other > 0
	pair<BigInt, BigInt> divmod(const BigInt& other) const {
		if (other == BigInt("0")) throw runtime_error("Division by zero");
		if (*this < other) return {BigInt("0"), *this};
		string a = digits;
		string b = other.digits;
		string quotient;
		BigInt remainder("0");
		for (char digit : a) {
			remainder = remainder.mul(BigInt("10")).add(BigInt(string(1, digit)));
			int q = 0;
			while (remainder >= other) {
				remainder = remainder.sub(other);
				++q;
			}
			quotient.push_back(char(q + '0'));
		}
		quotient.erase(0, quotient.find_first_not_of('0'));
		if (quotient.empty()) quotient = "0";
		return {BigInt(quotient), remainder};
	}
	
	// 取模
	BigInt mod(const BigInt& other) const {
		return divmod(other).second;
	}
	
	// 乘以10的幂
	BigInt mulPow10(int exp) const {
		if (digits == "0") return *this;
		string res = digits + string(exp, '0');
		return BigInt(res);
	}
	
	// 除以2（判断是否能整除，并修改原数）
	bool divBy2() {
		string newDigits;
		int carry = 0;
		for (char ch : digits) {
			int cur = carry * 10 + (ch - '0');
			newDigits.push_back(char(cur / 2 + '0'));
			carry = cur % 2;
		}
		if (carry != 0) return false;
		*this = BigInt(newDigits);
		return true;
	}
	
	// 除以5（判断是否能整除，并修改原数）
	bool divBy5() {
		string newDigits;
		int carry = 0;
		for (char ch : digits) {
			int cur = carry * 10 + (ch - '0');
			newDigits.push_back(char(cur / 5 + '0'));
			carry = cur % 5;
		}
		if (carry != 0) return false;
		*this = BigInt(newDigits);
		return true;
	}
	
	// 乘以2
	BigInt mul2() const {
		string a = digits;
		int carry = 0;
		string result;
		for (int i = a.length() - 1; i >= 0; --i) {
			int prod = (a[i] - '0') * 2 + carry;
			result.push_back(char(prod % 10 + '0'));
			carry = prod / 10;
		}
		if (carry) result.push_back(char(carry + '0'));
		reverse(result.begin(), result.end());
		return BigInt(result);
	}
	
	// 乘以5
	BigInt mul5() const {
		string a = digits;
		int carry = 0;
		string result;
		for (int i = a.length() - 1; i >= 0; --i) {
			int prod = (a[i] - '0') * 5 + carry;
			result.push_back(char(prod % 10 + '0'));
			carry = prod / 10;
		}
		if (carry) result.push_back(char(carry + '0'));
		reverse(result.begin(), result.end());
		return BigInt(result);
	}
	
	// 最大公约数（欧几里得算法）
	static BigInt gcd(const BigInt& a, const BigInt& b) {
		if (b == BigInt("0")) return a;
		return gcd(b, a.mod(b));
	}
};

// 分数类，支持带符号的分数（始终最简）
class Fraction {
private:
	int sign;       // 1 或 -1，0 时 sign=1
	BigInt num;     // 分子，非负
	BigInt den;     // 分母，正数
	
	// 约分
	void reduce() {
		if (num == BigInt("0")) {
			den = BigInt("1");
			sign = 1;
			return;
		}
		BigInt g = BigInt::gcd(num, den);
		num = num.divmod(g).first;
		den = den.divmod(g).first;
	}
	
public:
	// 默认构造为0
	Fraction() : sign(1), num("0"), den("1") {}
	
	// 从字符串构造（例如 "-123.456"）
	explicit Fraction(const string& s) {
		// 处理符号
		string str = s;
		sign = 1;
		if (!str.empty() && str[0] == '-') {
			sign = -1;
			str = str.substr(1);
		}
		// 分割整数和小数部分
		size_t dotPos = str.find('.');
		string intPart, fracPart;
		if (dotPos == string::npos) {
			intPart = str;
			fracPart = "";
		} else {
			intPart = str.substr(0, dotPos);
			fracPart = str.substr(dotPos + 1);
		}
		// 整数部分默认为"0"
		if (intPart.empty()) intPart = "0";
		// 去除整数部分前导零
		intPart.erase(0, intPart.find_first_not_of('0'));
		if (intPart.empty()) intPart = "0";
		// 小数部分去除末尾零
		while (!fracPart.empty() && fracPart.back() == '0')
			fracPart.pop_back();
		
		// 构造分数：分子 = intPart * 10^scale + fracPart，分母 = 10^scale
		BigInt intNum(intPart);
		if (fracPart.empty()) {
			num = intNum;
			den = BigInt("1");
		} else {
			BigInt fracNum(fracPart);
			int scale = fracPart.length();
			num = intNum.mulPow10(scale).add(fracNum);
			den = BigInt("1").mulPow10(scale);
		}
		reduce();
		// 如果分子为0，符号修正为+
		if (num == BigInt("0")) sign = 1;
	}
	
	// 构造分数（直接指定）
	Fraction(int s, const BigInt& n, const BigInt& d) : sign(s), num(n), den(d) {
		reduce();
	}
	
	// 加法
	Fraction add(const Fraction& other) const {
		BigInt newNum = num.mul(other.den).add(other.num.mul(den));
		BigInt newDen = den.mul(other.den);
		int newSign = sign; // 同号相加
		if (sign != other.sign) {
			// 异号，比较绝对值大小决定符号
			BigInt left = num.mul(other.den);
			BigInt right = other.num.mul(den);
			if (left < right) {
				newSign = other.sign;
				newNum = right.sub(left);
			} else {
				newSign = sign;
				newNum = left.sub(right);
			}
		} else {
			newNum = num.mul(other.den).add(other.num.mul(den));
		}
		return Fraction(newSign, newNum, newDen);
	}
	
	// 减法
	Fraction sub(const Fraction& other) const {
		Fraction negOther = other;
		negOther.sign = -other.sign;
		return add(negOther);
	}
	
	// 乘法
	Fraction mul(const Fraction& other) const {
		BigInt newNum = num.mul(other.num);
		BigInt newDen = den.mul(other.den);
		int newSign = sign * other.sign;
		return Fraction(newSign, newNum, newDen);
	}
	
	// 除法
	Fraction div(const Fraction& other) const {
		if (other.num == BigInt("0")) throw runtime_error("Division by zero");
		BigInt newNum = num.mul(other.den);
		BigInt newDen = den.mul(other.num);
		int newSign = sign * other.sign;
		return Fraction(newSign, newNum, newDen);
	}
	
	// 判断是否为有限小数（分母只含2和5因子）
	bool isFiniteDecimal() const {
		if (num == BigInt("0")) return true; // 0 视为有限小数
		BigInt d = den;
		// 反复除以2和5
		while (d != BigInt("1")) {
			BigInt temp = d;
			if (temp.divBy2()) continue;
			if (temp.divBy5()) continue;
			return false;
		}
		return true;
	}
	
	// 转换为小数串（当 isFiniteDecimal 为 true 时）
	string toDecimalString() const {
		if (num == BigInt("0")) return "0";
		// 计算整数部分和小数部分
		auto div = num.divmod(den);
		BigInt integer = div.first;
		BigInt remainder = div.second;
		string intStr = integer.toString();
		// 如果分数为负，添加负号
		string result = (sign == -1 ? "-" : "") + intStr;
		if (remainder == BigInt("0")) return result; // 整数
		
		result += ".";
		// 模拟除法求小数部分
		while (remainder != BigInt("0")) {
			remainder = remainder.mul(BigInt("10"));
			auto step = remainder.divmod(den);
			result += step.first.toString();
			remainder = step.second;
		}
		return result;
	}
	
	// 转换为分数串（当 isFiniteDecimal 为 false 时）
	string toFractionString() const {
		if (num == BigInt("0")) return "0";
		string result = (sign == -1 ? "-" : "") + num.toString();
		if (den != BigInt("1"))
			result += "/" + den.toString();
		return result;
	}
	
	// 统一输出：若有限小数输出小数，否则输出分数
	string toString() const {
		if (isFiniteDecimal())
			return toDecimalString();
		else
			return toFractionString();
	}
};

// 输入验证：检查字符串是否为合法数字，并确保整数部分<=100，小数部分<=60
bool validateInput(const string& s) {
	// 允许负号、数字、一个小数点
	regex pattern(R"(^-?\d*\.?\d+$)"); // 至少一个数字，整数部分可选
	if (!regex_match(s, pattern)) return false;
	
	string str = s;
	if (str[0] == '-') str = str.substr(1);
	size_t dotPos = str.find('.');
	string intPart, fracPart;
	if (dotPos == string::npos) {
		intPart = str;
		fracPart = "";
	} else {
		intPart = str.substr(0, dotPos);
		fracPart = str.substr(dotPos + 1);
	}
	// 整数部分长度检查
	if (intPart.empty()) intPart = "0"; // .123 视为整数部分0
	if (intPart.length() > 100) return false;
	// 小数部分长度检查
	if (fracPart.length() > 60) return false;
	return true;
}

// 主程序
int main() {
	cout << "高精度计算器（整数部分最多100位，小数部分最多60位）" << endl;
	while (true) {
		cout << "\n请选择模式：\n";
		cout << "1. 加法\n";
		cout << "2. 减法\n";
		cout << "3. 乘法\n";
		cout << "4. 除法\n";
		cout << "0. 退出\n";
		cout << "输入选项: ";
		int choice;
		cin >> choice;
		cin.ignore(); // 清除换行符
		
		if (choice == 0) break;
		if (choice < 1 || choice > 4) {
			cout << "无效选项，请重新输入。" << endl;
			continue;
		}
		
		string sa, sb;
		cout << "请输入第一个数: ";
		getline(cin, sa);
		if (!validateInput(sa)) {
			cout << "输入格式错误或超出位数限制，请重新输入。" << endl;
			continue;
		}
		cout << "请输入第二个数: ";
		getline(cin, sb);
		if (!validateInput(sb)) {
			cout << "输入格式错误或超出位数限制，请重新输入。" << endl;
			continue;
		}
		
		try {
			Fraction a(sa), b(sb);
			Fraction result;
			switch (choice) {
				case 1: result = a.add(b); break;
				case 2: result = a.sub(b); break;
				case 3: result = a.mul(b); break;
				case 4: result = a.div(b); break;
			}
			cout << "结果: " << result.toString() << endl;
		} catch (const exception& e) {
			cout << "错误: " << e.what() << endl;
		}
	}
	return 0;
}
