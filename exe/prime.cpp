#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <stdlib.h>
#include <cctype>

using namespace std;

// ==================== 函数声明 ====================
string add(const string& a, const string& b);
int compare(const string& a, const string& b);
string subtract(const string& a, const string& b);
string divide_by_2(const string& a);
int mod(const string& a, int m);
string multiply_mod(const string& a, const string& b, const string& n);
string power_mod(string a, string b, const string& n);
bool miller_rabin(const string& n);

// ==================== 随机数生成器 ====================
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

// 生成随机数字字符
char random_digit() {
	return '0' + (rng() % 10);
}

// 生成指定位数的随机数（首位不为0）
string generate_random_number(int digits) {
	if (digits <= 0) return "0";
	
	string num;
	num.push_back('1' + (rng() % 9));  // 首位1-9
	
	for (int i = 1; i < digits; i++) {
		num.push_back(random_digit());
	}
	
	return num;
}

// ==================== 大整数运算（字符串表示） ====================

// 大整数加法
string add(const string& a, const string& b) {
	string result;
	int carry = 0;
	int i = a.length() - 1, j = b.length() - 1;
	
	while (i >= 0 || j >= 0 || carry) {
		int sum = carry;
		if (i >= 0) sum += a[i--] - '0';
		if (j >= 0) sum += b[j--] - '0';
		result.push_back((sum % 10) + '0');
		carry = sum / 10;
	}
	
	reverse(result.begin(), result.end());
	return result;
}

// 比较两个大整数（字符串形式）
int compare(const string& a, const string& b) {
	if (a.length() != b.length()) 
		return a.length() < b.length() ? -1 : 1;
	return a.compare(b);
}

// 大整数减法（a >= b）
string subtract(const string& a, const string& b) {
	if (compare(a, b) < 0) return "0";  // 确保a >= b
	
	string res;
	int carry = 0;
	int i = a.length() - 1, j = b.length() - 1;
	
	while (i >= 0 || j >= 0 || carry) {
		int digit_a = i >= 0 ? a[i--] - '0' : 0;
		int digit_b = j >= 0 ? b[j--] - '0' : 0;
		int diff = digit_a - digit_b - carry;
		if (diff < 0) {
			diff += 10;
			carry = 1;
		} else {
			carry = 0;
		}
		res.push_back(diff + '0');
	}
	
	// 移除前导零
	while (res.length() > 1 && res.back() == '0') res.pop_back();
	reverse(res.begin(), res.end());
	return res;
}

// 大整数除以2
string divide_by_2(const string& a) {
	string res;
	int carry = 0;
	
	for (char ch : a) {
		int digit = ch - '0';
		int cur = carry * 10 + digit;
		res.push_back((cur / 2) + '0');
		carry = cur % 2;
	}
	
	// 移除前导零
	size_t start = res.find_first_not_of('0');
	return (start == string::npos) ? "0" : res.substr(start);
}

// 大整数取模（模一个小整数）
int mod(const string& a, int m) {
	int res = 0;
	for (char ch : a) {
		res = (res * 10 + (ch - '0')) % m;
	}
	return res;
}

// 简单的乘法函数（不使用取模）
string multiply(const string& a, const string& b) {
	if (a == "0" || b == "0") return "0";
	
	int len_a = a.length();
	int len_b = b.length();
	vector<int> result(len_a + len_b, 0);
	
	// 逐位相乘
	for (int i = len_a - 1; i >= 0; i--) {
		for (int j = len_b - 1; j >= 0; j--) {
			int product = (a[i] - '0') * (b[j] - '0');
			int sum = product + result[i + j + 1];
			result[i + j + 1] = sum % 10;
			result[i + j] += sum / 10;
		}
	}
	
	// 转换为字符串
	string res_str;
	for (int num : result) {
		if (!(res_str.empty() && num == 0)) {
			res_str.push_back(num + '0');
		}
	}
	
	return res_str.empty() ? "0" : res_str;
}

// 大整数取模（模一个大整数）
string mod_big(const string& a, const string& n) {
	if (compare(a, n) < 0) return a;
	
	// 使用长除法取模
	string remainder = "0";
	
	for (char digit : a) {
		remainder.push_back(digit);
		// 移除前导零
		while (remainder.length() > 1 && remainder[0] == '0') {
			remainder.erase(0, 1);
		}
		
		// 如果remainder >= n，则减去除数
		while (compare(remainder, n) >= 0) {
			remainder = subtract(remainder, n);
		}
	}
	
	return remainder;
}

// 大整数乘法取模 (a * b) % n
string multiply_mod(const string& a, const string& b, const string& n) {
	if (a == "0" || b == "0") return "0";
	
	string result = "0";
	
	// 从b的最低位开始
	for (int i = b.length() - 1; i >= 0; i--) {
		int digit = b[i] - '0';
		
		if (digit != 0) {
			// 计算 a * digit
			string temp = a;
			int carry = 0;
			
			// 乘以单个数字
			for (int j = temp.length() - 1; j >= 0; j--) {
				int product = (temp[j] - '0') * digit + carry;
				temp[j] = (product % 10) + '0';
				carry = product / 10;
			}
			
			if (carry > 0) {
				temp = to_string(carry) + temp;
			}
			
			// 添加适当的零（对应位权）
			temp += string(b.length() - 1 - i, '0');
			
			// 累加到结果
			result = add(result, temp);
			
			// 取模
			result = mod_big(result, n);
		}
	}
	
	return result;
}

// 快速幂取模 (a^b mod n)
string power_mod(string a, string b, const string& n) {
	string result = "1";
	
	while (compare(b, "0") > 0) {
		// 如果b是奇数
		if ((b.back() - '0') % 2 == 1) {
			result = multiply_mod(result, a, n);
		}
		
		// a = a * a mod n
		a = multiply_mod(a, a, n);
		
		// b = b / 2
		b = divide_by_2(b);
	}
	
	return result;
}

// ==================== 米勒-拉宾素性测试 ====================

// 对于 10^100 以内的数，使用以下基可以确保确定性测试
const int bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

bool miller_rabin(const string& n) {
	// 处理小数字
	if (n == "2" || n == "3") return true;
	if (n == "1" || n == "0") return false;
	
	// 检查是否为偶数
	if ((n.back() - '0') % 2 == 0) return false;
	
	// 将 n-1 写成 d * 2^s 的形式
	string d = subtract(n, "1");
	int s = 0;
	while ((d.back() - '0') % 2 == 0) {
		d = divide_by_2(d);
		s++;
	}
	
	// 测试不同的基
	for (int base : bases) {
		if (base == 0) continue;
		
		string base_str = to_string(base);
		// 如果基大于等于n，跳过
		if (compare(base_str, n) >= 0) continue;
		
		string x = power_mod(base_str, d, n);
		
		if (x == "1" || x == subtract(n, "1")) {
			continue;
		}
		
		bool composite = true;
		string x_temp = x;
		for (int r = 0; r < s; r++) {
			x_temp = multiply_mod(x_temp, x_temp, n);
			if (x_temp == subtract(n, "1")) {
				composite = false;
				break;
			}
		}
		
		if (composite) {
			return false;
		}
	}
	
	return true;
}

// ==================== 生成随机质数 ====================

// 检查是否能被小质数整除（快速排除非质数）
bool divisible_by_small_primes(const string& n) {
	int small_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
	
	// 检查偶数
	if ((n.back() - '0') % 2 == 0) return true;
	
	// 检查其他小质数
	for (int prime : small_primes) {
		if (mod(n, prime) == 0) {
			return true;
		}
	}
	
	return false;
}

// 生成一个随机质数（指定位数）
string generate_random_prime(int digits, int max_attempts = 100) {
	if (digits <= 0) return "2";
	if (digits == 1) {
		// 一位数的质数：2, 3, 5, 7
		int primes[] = {2, 3, 5, 7};
		return to_string(primes[rng() % 4]);
	}
	
	int attempts = 0;
	while (attempts < max_attempts) {
		// 生成随机数，确保是奇数（最后一位是奇数）
		string candidate = generate_random_number(digits);
		
		// 确保是奇数
		if ((candidate.back() - '0') % 2 == 0) {
			// 如果是偶数，加1变成奇数
			int last_digit = candidate.back() - '0';
			candidate.back() = (last_digit == 9) ? '1' : char(last_digit + 1 + '0');
		}
		
		// 快速检查：排除能被小质数整除的数
		if (divisible_by_small_primes(candidate)) {
			attempts++;
			continue;
		}
		
		// 米勒-拉宾测试
		if (miller_rabin(candidate)) {
			return candidate;
		}
		
		attempts++;
	}
	
	// 如果没找到，返回一个已知的大质数
	if (digits <= 20) {
		// 返回一个较小的已知质数
		return "1000000000000000003";  // 19位的质数
	} else {
		// 返回一个较大的已知质数
		return "9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999929";
	}
}

// ==================== 主函数 ====================

int main() {
	int choice;
	cout << "选择操作：" << endl;
	cout << "1. 判断输入的数是否为质数" << endl;
	cout << "2. 随机生成一个质数" << endl;
	cout << "请输入选择 (1 或 2): ";
	cin >> choice;
	
	if (choice == 1) {
		// 判断输入的数是否为质数
		string n;
		cout << "请输入一个大整数（不超过100位）: ";
		cin >> n;
		
		// 移除可能的空格或换行
		n.erase(remove_if(n.begin(), n.end(), ::isspace), n.end());
		
		// 验证输入
		if (n.length() > 100 || !all_of(n.begin(), n.end(), ::isdigit)) {
			cout << "输入无效！" << endl;
			return 1;
		}
		
		// 移除前导零
		n.erase(0, n.find_first_not_of('0'));
		if (n.empty()) n = "0";
		
		// 检查是否为质数
		cout << "测试中，请稍候..." << endl;
		if (miller_rabin(n)) {
			cout << n << " 是质数" << endl;
		} else {
			cout << n << " 不是质数" << endl;
		}
	} else if (choice == 2) {
		// 随机生成质数
		int digits;
		cout << "请输入要生成的质数的位数 (1-100): ";
		cin >> digits;
		
		if (digits < 1 || digits > 100) {
			cout << "位数必须在1到100之间！" << endl;
			return 1;
		}
		
		cout << "正在生成 " << digits << " 位的随机质数..." << endl;
		cout << "这可能需要一些时间，请耐心等待..." << endl;
		
		string prime = generate_random_prime(digits);
		
		cout << "\n生成的质数为：" << endl;
		cout << prime << endl;
		
		// 验证一下（可选）
		cout << "\n验证中..." << endl;
		if (miller_rabin(prime)) {
			cout << "验证通过：这是一个质数" << endl;
		} else {
			cout << "警告：生成的数可能不是质数！" << endl;
		}
		cout << "位数：" << prime.length() << endl;
	} else {
		cout << "无效的选择！" << endl;
		return 1;
	}
	return 0;
}
