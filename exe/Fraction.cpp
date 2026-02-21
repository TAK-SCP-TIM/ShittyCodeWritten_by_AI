#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <stdlib.h>
using namespace std;

// ==================== 大整数工具函数（非负）====================

string removeLeadingZeros(string s) {
	size_t pos = s.find_first_not_of('0');
	if (pos == string::npos) return "0";
	return s.substr(pos);
}

int compare(const string& a, const string& b) {
	string sa = removeLeadingZeros(a);
	string sb = removeLeadingZeros(b);
	if (sa.length() != sb.length())
		return sa.length() < sb.length() ? -1 : 1;
	for (size_t i = 0; i < sa.length(); ++i) {
		if (sa[i] != sb[i])
			return sa[i] < sb[i] ? -1 : 1;
	}
	return 0;
}

string subtract(const string& a, const string& b) {
	string result;
	int borrow = 0;
	int i = a.length() - 1, j = b.length() - 1;
	while (i >= 0 || j >= 0) {
		int digitA = i >= 0 ? a[i--] - '0' : 0;
		int digitB = j >= 0 ? b[j--] - '0' : 0;
		int diff = digitA - digitB - borrow;
		if (diff < 0) {
			diff += 10;
			borrow = 1;
		} else {
			borrow = 0;
		}
		result.push_back(diff + '0');
	}
	reverse(result.begin(), result.end());
	return removeLeadingZeros(result);
}

string multiplyByDigit(const string& a, int d) {
	if (d == 0) return "0";
	string result;
	int carry = 0;
	for (int i = a.length() - 1; i >= 0; --i) {
		int prod = (a[i] - '0') * d + carry;
		result.push_back(prod % 10 + '0');
		carry = prod / 10;
	}
	if (carry) result.push_back(carry + '0');
	reverse(result.begin(), result.end());
	return result;
}

pair<string, string> divide(const string& dividend, const string& divisor) {
	if (divisor == "0") {
		cerr << "错误：除数为零！" << endl;
		exit(1);
	}
	if (compare(dividend, divisor) < 0) {
		return {"0", dividend};
	}
	string quotient;
	string remainder;
	for (char c : dividend) {
		remainder.push_back(c);
		remainder = removeLeadingZeros(remainder);
		int digit = 0;
		if (compare(remainder, divisor) >= 0) {
			for (int d = 9; d >= 0; --d) {
				string prod = multiplyByDigit(divisor, d);
				if (compare(remainder, prod) >= 0) {
					digit = d;
					break;
				}
			}
			string prod = multiplyByDigit(divisor, digit);
			remainder = subtract(remainder, prod);
			remainder = removeLeadingZeros(remainder);
		} else {
			digit = 0;
		}
		quotient.push_back(digit + '0');
	}
	quotient = removeLeadingZeros(quotient);
	return {quotient, remainder};
}

bool divisibleBySmall(const string& s, int d) {
	if (d == 2) return (s.back() - '0') % 2 == 0;
	else { // d == 5
		int last = s.back() - '0';
		return last == 0 || last == 5;
	}
}

string divideBySmall(const string& s, int d) {
	string result;
	int remainder = 0;
	for (char c : s) {
		int cur = remainder * 10 + (c - '0');
		result.push_back(cur / d + '0');
		remainder = cur % d;
	}
	return removeLeadingZeros(result);
}

// ==================== 模式1：小数转分数（支持负数）====================

void decimalToFraction() {
	cout << "\n--- 小数转最简分数 ---\n";
	cout << "请输入一个小数（整数部分≤70位，小数部分≤70位，可带负号，例如 -123.456）：\n";
	string s;
	getline(cin, s);
	s.erase(remove(s.begin(), s.end(), ' '), s.end());
	
	// 处理符号
	bool negative = false;
	if (!s.empty() && s[0] == '-') {
		negative = true;
		s = s.substr(1);
	}
	
	string numStr, denStr;
	size_t dot = s.find('.');
	if (dot == string::npos) {
		numStr = s;
		denStr = "1";
	} else {
		string intPart = s.substr(0, dot);
		string fracPart = s.substr(dot + 1);
		if (intPart.empty()) intPart = "0";
		if (fracPart.empty()) fracPart = "0";
		int n = fracPart.length();
		numStr = intPart + fracPart;
		denStr = "1" + string(n, '0');
	}
	
	numStr = removeLeadingZeros(numStr);
	
	// 零值处理（忽略符号）
	if (numStr == "0") {
		cout << "0/1" << endl;
		return;
	}
	
	// 约简
	while (divisibleBySmall(numStr, 2) && divisibleBySmall(denStr, 2)) {
		numStr = divideBySmall(numStr, 2);
		denStr = divideBySmall(denStr, 2);
	}
	while (divisibleBySmall(numStr, 5) && divisibleBySmall(denStr, 5)) {
		numStr = divideBySmall(numStr, 5);
		denStr = divideBySmall(denStr, 5);
	}
	
	if (negative)
		cout << "-" << numStr << "/" << denStr << endl;
	else
		cout << numStr << "/" << denStr << endl;
}

// ==================== 模式2：分数转小数（支持负数）====================

void fractionToDecimal() {
	cout << "\n--- 分数转小数 ---\n";
	cout << "请输入分子和分母（整数，最多100位，分母≠0，可带负号）：\n";
	cout << "分子：";
	string numInput, denInput;
	getline(cin, numInput);
	numInput.erase(remove(numInput.begin(), numInput.end(), ' '), numInput.end());
	cout << "分母：";
	getline(cin, denInput);
	denInput.erase(remove(denInput.begin(), denInput.end(), ' '), denInput.end());
	
	// 处理符号
	bool numNegative = false, denNegative = false;
	if (!numInput.empty() && numInput[0] == '-') {
		numNegative = true;
		numInput = numInput.substr(1);
	}
	if (!denInput.empty() && denInput[0] == '-') {
		denNegative = true;
		denInput = denInput.substr(1);
	}
	
	// 检查分母是否为0
	if (denInput == "0") {
		cout << "错误：分母不能为零！" << endl;
		return;
	}
	
	// 确定最终符号
	bool resultNegative = (numNegative != denNegative); // 异号为负
	// 分子绝对值可能为0，则结果不考虑符号
	if (numInput == "0" || numInput.empty()) {
		cout << "0" << endl;
		return;
	}
	
	// 使用绝对值进行运算
	auto [integer, rem] = divide(numInput, denInput);
	if (rem == "0") {
		// 整除
		if (resultNegative)
			cout << "-" << integer << endl;
		else
			cout << integer << endl;
		return;
	}
	
	// 计算小数部分
	string decimalPart;
	unordered_map<string, int> seen;
	seen[rem] = 0;
	int position = 0;
	
	while (true) {
		rem = rem + "0";
		auto [digitStr, newRem] = divide(rem, denInput);
		char digit = digitStr[0];
		decimalPart.push_back(digit);
		position++;
		
		if (newRem == "0") {
			// 有限小数
			if (resultNegative)
				cout << "-" << integer << "." << decimalPart << endl;
			else
				cout << integer << "." << decimalPart << endl;
			return;
		}
		
		if (seen.count(newRem)) {
			int start = seen[newRem];
			string nonRepeating = decimalPart.substr(0, start);
			string repeating = decimalPart.substr(start);
			if (resultNegative)
				cout << "-" << integer << "." << nonRepeating << "(" << repeating << ")" << endl;
			else
				cout << integer << "." << nonRepeating << "(" << repeating << ")" << endl;
			return;
		}
		
		seen[newRem] = position;
		rem = newRem;
	}
}

// ==================== 主程序 ====================

int main() {
	cout << "========================================\n";
	cout << "   高精度小数与最简分数双向转换（支持负数）\n";
	cout << "========================================\n";
	cout << "请选择模式：\n";
	cout << "  1 - 小数 -> 最简分数（整数≤70位，小数≤70位）\n";
	cout << "  2 - 分数 -> 小数（分子、分母≤100位，分母≠0）\n";
	cout << "输入数字（1或2）：";
	
	string choiceStr;
	getline(cin, choiceStr);
	choiceStr.erase(remove(choiceStr.begin(), choiceStr.end(), ' '), choiceStr.end());
	
	if (choiceStr == "1") {
		decimalToFraction();
	} else if (choiceStr == "2") {
		fractionToDecimal();
	} else {
		cout << "无效选择，请运行程序后输入1或2。" << endl;
	}
	
	return 0;
}
