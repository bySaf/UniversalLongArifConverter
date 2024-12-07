#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <utility>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <cctype>
#include <cmath>

std::string to_string(int num){
  std::string ans;
  while (num){
    ans+=static_cast<char>(48 + num%10);
    num/=10;
  }
  std::reverse(ans.begin(), ans.end());
  return ans;
}


class BigInteger {
 private:
  std::vector<int> digits;
  int base;
  int sign;
  void trim() {
    while (digits.size() > 1 && digits.back() == 0)
      digits.pop_back();
    if (digits.size() == 1 && digits[0] == 0)
      sign = 1;
  }

  static bool abs_less(const BigInteger &a, const BigInteger &b) {
    if (a.digits.size() != b.digits.size())
      return a.digits.size() < b.digits.size();
    for (int i = a.digits.size() - 1; i >= 0; --i) {
      if (a.digits[i] != b.digits[i])
        return a.digits[i] < b.digits[i];
    }
    return false;
  }

  void add_abs(const BigInteger &other) {
    int carry = 0;
    size_t n = std::max(digits.size(), other.digits.size());
    digits.resize(n, 0);
    for (size_t i = 0; i < n || carry; ++i) {
      if (i >= digits.size())
        digits.push_back(0);
      int other_digit = (i < other.digits.size()) ? other.digits[i] : 0;
      long long sum = static_cast<long long>(digits[i]) + other_digit + carry;
      carry = (sum >= base) ? 1 : 0;
      digits[i] = static_cast<int>(sum % base);
    }
  }

  void sub_abs(const BigInteger &other) {
    int carry = 0;
    for (size_t i = 0; i < other.digits.size() || carry; ++i) {
      long long diff = static_cast<long long>(digits[i]) - ((i < other.digits.size()) ? other.digits[i] : 0) - carry;
      carry = 0;
      if (diff < 0) {
        carry = 1;
        diff += base;
      }
      digits[i] = static_cast<int>(diff);
    }
    trim();
  }

  void mul_small(int v) {
    if (v < 0) {
      sign = -sign;
      v = -v;
    }
    long long carry = 0;
    for (size_t i = 0; i < digits.size() || carry; ++i) {
      if (i == digits.size())
        digits.push_back(0);
      long long product = static_cast<long long>(digits[i]) * v + carry;
      digits[i] = static_cast<int>(product % base);
      carry = product / base;
    }
    trim();
  }

  int div_small(int v) {
    if (v == 0)
      throw std::invalid_argument("Division by zero");

    int original_sign = sign;
    if ((v < 0 && sign > 0) || (v > 0 && sign < 0))
      sign = -sign;

    v = std::abs(v);
    long long rem = 0;
    for (int i = static_cast<int>(digits.size()) - 1; i >= 0; --i) {
      long long current = rem * static_cast<long long>(base) + digits[i];
      digits[i] = static_cast<int>(current / v);
      rem = current % v;
    }
    trim();
    sign = original_sign;
    return static_cast<int>(rem);
  }

  std::vector<int> karatsubaMultiply(const std::vector<int> &a, const std::vector<int> &b) const {
    long n = a.size(), m = b.size();
    if (n == 0 || m == 0)
      return {0};

    if (n < 32 || m < 32) {
      std::vector<int> res(n + m, 0);
      for (size_t i = 0; i < n; ++i) {
        long long carry = 0;
        for (size_t j = 0; j < m || carry; ++j) {
          long long product = static_cast<long long>(res[i + j]) +
              (static_cast<long long>(a[i]) * (j < m ? b[j] : 0)) +
              carry;
          res[i + j] = static_cast<int>(product % base);
          carry = product / base;
        }
      }
      while (!res.empty() && res.back() == 0)
        res.pop_back();
      return res;
    }

    long k = std::min(n, m) / 2;

    std::vector<int> a1(a.begin(), a.begin() + k);
    std::vector<int> a2(a.begin() + k, a.end());

    std::vector<int> b1(b.begin(), b.begin() + k);
    std::vector<int> b2(b.begin() + k, b.end());

    std::vector<int> a1b1 = karatsubaMultiply(a1, b1);
    std::vector<int> a2b2 = karatsubaMultiply(a2, b2);

    std::vector<int> a1a2 = a1;
    add_vectors(a1a2, a2, base);
    std::vector<int> b1b2 = b1;
    add_vectors(b1b2, b2, base);

    std::vector<int> r = karatsubaMultiply(a1a2, b1b2);

    sub_vectors(r, a1b1, base);
    sub_vectors(r, a2b2, base);

    std::vector<int> res(a1b1.size() + 2 * k, 0);
    add_vectors(res, a1b1, 0);
    add_vectors(res, r, k);
    add_vectors(res, a2b2, 2 * k);

    while (!res.empty() && res.back() == 0)
      res.pop_back();

    return res;
  }

  static void add_vectors(std::vector<int> &a, const std::vector<int> &b, int base) {
    long long carry = 0;
    size_t n = std::max(a.size(), b.size());
    a.resize(n, 0);
    for (size_t i = 0; i < b.size() || carry; ++i) {
      if (i >= a.size())
        a.push_back(0);
      int b_digit = (i < b.size()) ? b[i] : 0;
      long long sum = static_cast<long long>(a[i]) + b_digit + carry;
      a[i] = static_cast<int>(sum % base);
      carry = sum / base;
    }
  }

  static void sub_vectors(std::vector<int> &a, const std::vector<int> &b, int base) {
    int carry = 0;
    for (size_t i = 0; i < b.size() || carry; ++i) {
      long long diff = static_cast<long long>(a[i]) - ((i < b.size()) ? b[i] : 0) - carry;
      carry = 0;
      if (diff < 0) {
        carry = 1;
        diff += base;
      }
      a[i] = static_cast<int>(diff);
    }
    while (a.size() > 1 && a.back() == 0)
      a.pop_back();
  }

 public:

  BigInteger() : digits(1, 0), base(10), sign(1) {}

  BigInteger(int v, int input_base = 10) : digits(), base(input_base), sign(1) {
    if (v < 0) {
      sign = -1;
      v = -v;
    }
    if (v == 0) {
      digits.push_back(0);
    } else {
      while (v > 0) {
        digits.push_back(v % base);
        v /= base;
      }
    }
  }

  BigInteger(const std::string &s, int input_base = 10) : digits(), base(input_base), sign(1) {
    read(s);
  }

  [[nodiscard]] int get_base() const {
    return base;
  }

  void read(const std::string &sex) {
    std::string s = sex;
    int temp_ = 0;
    for (auto x : s) {
      s[temp_] = char(toupper(x));
      temp_++;
    }
    digits.clear();
    sign = 1;
    int pos = 0;
    if (s[0] == '-') {
      sign = -1;
      pos++;
    }
    int f = 0, sum = 0, c = 1;
    std::vector<int> temp;

    for (int temp_index = static_cast<int>(s.size() - 1); temp_index >= pos; --temp_index) {
      if (s[temp_index] == ']') {
        temp.clear();
        f = 1;
        continue;
      }
      if (s[temp_index] == '[') {
        f = 0;

        for (auto i : temp) {
          sum += i * c;
          c *= 10;
        }

        if (sum < base) {
          std::cout << "xz: " << sum << std::endl;
          digits.push_back(sum);
          temp.clear();
          sum = 0;
          c = 1;
          continue;
        } else if (sum < 10){
          throw std::invalid_argument("Нормальное число вводи блин, давай с 10 хотя бы");
        } else {
          throw std::invalid_argument("Digit out of range for the base2");
        }

      }
      if (f) {
        temp.push_back(s[temp_index] - '0');
        continue;
      }
      if (!std::isdigit(s[temp_index])) {
        if (s[temp_index] - 'A' >= 0) {
          if (int(s[temp_index] - 'A' + 10) < base) {
            digits.push_back(int(s[temp_index] - 'A' + 10));
            continue;
          } else {
            throw std::invalid_argument("Digit out of range for the base");
          }
        } else {
          throw std::invalid_argument("Invalid character in input string");
        }
      }
      int digit = s[temp_index] - '0';
      if (digit >= base) throw std::invalid_argument("Digit out of range for thebase");
      digits.push_back(digit);
    }

    trim();
  }

  BigInteger operator+() const {
    return *this;
  }

  BigInteger operator-() const {
    BigInteger res = *this;
    if (!res.is_zero())
      res.sign = -res.sign;
    return res;
  }

  BigInteger operator+(const BigInteger &other) const {

    BigInteger result;
    result.base = base;

    if (sign == other.sign) {
      result.sign = sign;
      result.digits = digits;
      result.add_abs(other);
    } else {
      if (abs_less(*this, other)) {
        result.sign = other.sign;
        result.digits = other.digits;
        result.sub_abs(*this);
      } else {
        result.sign = sign;
        result.digits = digits;
        result.sub_abs(other);
      }
    }
    return result;
  }

  BigInteger operator-(const BigInteger &other) const {
    if (base != other.base)
      throw std::invalid_argument("Bases must be the same for subtraction");

    BigInteger result;
    result.base = base;

    if (sign != other.sign) {
      result.sign = sign;
      result.digits = digits;
      result.add_abs(other);
    } else {
      if (abs_less(*this, other)) {
        result.sign = -sign;
        result.digits = other.digits;
        result.sub_abs(*this);
      } else {
        result.sign = sign;
        result.digits = digits;
        result.sub_abs(other);
      }
    }
    return result;
  }

  BigInteger operator*(const BigInteger &other) const {
    if (base != other.base)
      BigInteger kek = other.convert_to_bigint(base);

    BigInteger result;
    result.base = base;
    result.sign = sign * other.sign;
    result.digits = karatsubaMultiply(digits, other.digits);

    if (result.digits.empty())
      result.digits.push_back(0);

    return result;
  }

  BigInteger operator%(const BigInteger &other) const {
    BigInteger result = *this - other * (*this / other);
    result.base = other.base;
    return result;
  }

  BigInteger operator/(const BigInteger &other) const {

    if (base != other.base)
      throw std::invalid_argument("Bases must be the same for division");
    if (other.is_zero())
      throw std::invalid_argument("Division by zero");

    BigInteger dividend = this->abs();
    BigInteger divisor = other.abs();

    BigInteger quotient(0, base);
    BigInteger left(0, base);
    BigInteger determinator1(1, base);
    BigInteger right(dividend.convert_to_string(base), base);
    right = right + determinator1;
    quotient.sign = sign * other.sign;

    BigInteger remainder = BigInteger(0, base);

    while (left + determinator1 < right) {
      quotient = (left + right);
      quotient.div_small(2);
      if (quotient * divisor <= dividend) {
        left = quotient;
      } else {
        right = quotient;
      }
      quotient.trim();
      left.trim();
      right.trim();

    }

    left.trim();
    return left;
  }

  bool operator<(const BigInteger &other) const {
    if (base != other.base)
      throw std::invalid_argument("Bases must be the same for comparison");

    if (sign != other.sign)
      return sign < other.sign;

    if (sign == 1) {
      if (digits.size() != other.digits.size())
        return digits.size() < other.digits.size();
      for (int i = static_cast<int>(digits.size() - 1); i >= 0; --i) {
        if (digits[i] != other.digits[i])
          return digits[i] < other.digits[i];
      }
      return false;
    } else {
      if (digits.size() != other.digits.size())
        return digits.size() > other.digits.size();
      for (int i = static_cast<int>(digits.size() - 1); i >= 0; --i) {
        if (digits[i] != other.digits[i])
          return digits[i] > other.digits[i];
      }
      return false;
    }
  }

  bool operator>(const BigInteger &other) const {
    return other < *this;
  }

  bool operator<=(const BigInteger &other) const {
    return !(other < *this);
  }

  bool operator>=(const BigInteger &other) const {
    return !(*this < other);
  }

  bool operator==(const BigInteger &other) const {
    if (base != other.base)
      BigInteger new_other = other.convert_to_bigint(base);

    return (sign == other.sign) && (digits == other.digits);
  }

  bool operator!=(const BigInteger &other) const {
    return !(*this == other);
  }

  bool operator==(const int &other) const {
    BigInteger temp(other);
    BigInteger tempy = temp.convert_to_bigint(base);
    return (*this == tempy);
  }

  bool operator!=(const int &other) const {
    return !(*this == other);
  }

  [[nodiscard]] BigInteger abs() const {
    BigInteger result = *this;
    result.sign = 1;
    return result;
  }

  [[nodiscard]] bool is_zero() const {
    return digits.size() == 1 && digits[0] == 0;
  }

  [[nodiscard]] std::string convert_to_string(int new_base) const {

    if (is_zero())
      return "0";

    BigInteger temp = this->abs();
    std::string result;
    while (!temp.is_zero()) {
      int rem = temp.div_small(new_base);
      if (rem < 10) {
        result += static_cast<char>('0' + rem);
      }else if (rem > 36) {
        std::string i_hate_my_life = to_string(rem);
        std::reverse(i_hate_my_life.begin(), i_hate_my_life.end());
        result += (']' + i_hate_my_life + '[');
      }else {
        result += static_cast<char>('A' + (rem - 10));
      }
    }
    if (sign == -1)
      result += '-';

    std::reverse(result.begin(), result.end());
    return result;
  }

  [[nodiscard]] BigInteger convert_to_bigint(int new_base) const {
    BigInteger temp(this->convert_to_string(new_base), new_base);
    return temp;
  }

  friend std::istream &operator>>(std::istream &in, BigInteger &number) {
    std::string s;
    in >> s;
    number.read(s);
    return in;
  }

  friend std::ostream &operator<<(std::ostream &out, const BigInteger &number) {
    if (number.sign == -1)
      out << '-';
    if (number.is_zero()) {
      out << '0';
      return out;
    }

    for (int i = static_cast<int>(number.digits.size()) - 1; i >= 0; --i) {
      if (number.digits[i] > 9 and number.digits[i] <= 35) out << char('A' - 10 + number.digits[i]);
      else if (number.digits[i] > 35) {
        out << '[';
        out << number.digits[i];
        out << ']';
      } else out << number.digits[i];
    }
    return out;
  }

  friend int get_len(const BigInteger &number, int cnt = 0){
    for (auto i : number.digits) cnt++;
    return cnt;
  }
};

BigInteger gcd(const BigInteger &first_number, const BigInteger& second_number) {
  return (second_number != 0) ? gcd(second_number, first_number % second_number) : first_number;
}

class BigFraction {
 public:
  BigFraction() {
    num_ = 0;
    den_ = 1;
    base = 10;
  };

  BigFraction(const std::string& s, int base, bool period = false) {
    if (!period) {
      BigInteger temp_num(s, base);
      int cnt = 0;
      for (auto i : s) if (i == '[' or i == ']') cnt += 1;
      BigInteger temp_den(static_cast<int>(std::pow(base, get_len(temp_num))), base);
      std::cout << s << " zozz: " << temp_num << " " << temp_den << " " << get_len(temp_num) + 1 << std::endl;
      Simplify(temp_num, temp_den);
    }
    base = base;

  };

  void Simplify(BigInteger temp_num, BigInteger temp_den) {
    BigInteger nod = gcd(temp_num.convert_to_bigint(10), temp_den.convert_to_bigint(10));
    nod = nod.convert_to_bigint(temp_num.get_base());

    temp_num = temp_num / nod;
    temp_den = temp_den / nod;
    num_ = temp_num;
    den_ = temp_den;
    base = temp_num.get_base();
  }

  static std::pair<BigInteger, BigInteger> Get_Simplify(BigInteger temp_num, BigInteger temp_den) {
    BigInteger nod = gcd(temp_num.convert_to_bigint(10), temp_den.convert_to_bigint(10));
    nod = nod.convert_to_bigint(temp_num.get_base());

    temp_num = temp_num / nod;
    temp_den = temp_den / nod;
    return {temp_num, temp_den};
  }

  BigFraction(BigInteger num, BigInteger den) {
    Simplify(std::move(num), std::move(den));
  };

  explicit BigFraction(BigInteger num) : num_(std::move(num)) {};
  BigFraction &operator=(const BigFraction &other) = default;
  BigFraction(const BigFraction &other) = default;

  ~BigFraction() {
    num_ = 1;
    den_ = 1;
  };

  [[nodiscard]] BigInteger num() const {
    return num_;
  }
  [[nodiscard]] BigInteger den() const {
    return den_;
  }

  friend BigFraction operator+(const BigFraction &lhs, const BigFraction &rhs) {
    BigInteger new_den = (lhs.den_ * rhs.den_) / gcd(lhs.den_, rhs.den_);
    BigInteger new_num = lhs.num_ * new_den / lhs.den_ + rhs.num_ * new_den / rhs.den_;
    std::pair<BigInteger, BigInteger> ans = Get_Simplify(new_num, new_den);
    return {ans.first, ans.second};
  };

  friend BigFraction operator-(const BigFraction &lhs, const BigFraction &rhs) {
    BigInteger new_den = (lhs.den_ * rhs.den_) / gcd(lhs.den_, rhs.den_);
    BigInteger new_num = lhs.num_ * rhs.den_ - rhs.num_ * lhs.den_;
    std::pair<BigInteger, BigInteger> ans = Get_Simplify(new_num, new_den);
    return {ans.first, ans.second};
  };

  friend BigFraction operator*(const BigFraction &lhs, const BigFraction &rhs) {
    BigInteger new_den = lhs.den_ * rhs.den_;
    BigInteger new_num = lhs.num_ * rhs.num_;
    std::pair<BigInteger, BigInteger> ans = Get_Simplify(new_num, new_den);
    return {ans.first, ans.second};
  };

  friend BigFraction operator/(const BigFraction &lhs, const BigFraction &rhs) {
    BigInteger new_rhs_num = rhs.num_, new_rhs_den = rhs.den_;
    std::swap(new_rhs_num, new_rhs_den);
    BigInteger new_den = lhs.den_ * new_rhs_den;
    BigInteger new_num = lhs.num_ * new_rhs_num;
    std::pair<BigInteger, BigInteger> ans = Get_Simplify(new_num, new_den);
    return {ans.first, ans.second};
  };

  BigFraction &operator++() {
    num_ = num_ + den_;
    return *this;
  };

  BigFraction &operator--() {
    num_ = num_ - den_;
    return *this;
  };

  BigFraction operator++(const int) {
    BigFraction start = *this;
    num_ = num_ + den_;
    return start;
  };

  BigFraction operator--(const int) {
    BigFraction start = *this;
    num_ = num_ - den_;
    return start;
  };

  friend BigFraction operator+(BigInteger first, const BigFraction &rhs) {
    return (BigFraction(std::move(first)) + rhs);
  };

  friend BigFraction operator-(BigInteger first, const BigFraction &rhs) {
    return (BigFraction(std::move(first)) - rhs);
  };

  friend BigFraction operator*(BigInteger first, const BigFraction &rhs) {
    return (BigFraction(std::move(first)) * rhs);
  };

  friend BigFraction operator/(BigInteger first, const BigFraction &rhs) {
    return (BigFraction(std::move(first)) / rhs);
  };

  friend BigFraction operator+(const BigFraction &lhs, BigInteger second) {
    return (lhs + BigFraction(std::move(second)));
  };


  friend BigFraction operator-(const BigFraction &lhs, BigInteger second) {
    return (lhs - BigFraction(std::move(second)));
  };

  friend BigFraction operator*(const BigFraction &lhs, BigInteger second) {
    return (lhs * BigFraction(std::move(second)));
  };

  friend BigFraction operator/(const BigFraction &lhs, BigInteger second) {
    return (lhs / BigFraction(std::move(second)));
  };

  friend bool operator>(const BigFraction &lhs, const BigFraction &rhs) {
    BigFraction temp(lhs - rhs);
    return temp.num_ * temp.den_ > 0;
  };

  friend bool operator>=(const BigFraction &lhs, const BigFraction &rhs) {
    BigFraction temp(lhs - rhs);
    return temp.num_ * temp.den_ >= 0;
  };

  friend bool operator<(const BigFraction &lhs, const BigFraction &rhs) {
    BigFraction temp(lhs - rhs);
    return temp.num_ * temp.den_ < 0;
  };

  friend bool operator<=(const BigFraction &lhs, const BigFraction &rhs) {
    BigFraction temp(lhs - rhs);
    return temp.num_ * temp.den_ <= 0;
  };

  friend bool operator>(const BigFraction &lhs, BigInteger second) {
    BigFraction temp(lhs - BigFraction(std::move(second)));
    return temp.num_ * temp.den_ > 0;
  };

  friend bool operator>=(const BigFraction &lhs, BigInteger second) {
    BigFraction temp(lhs - BigFraction(std::move(second)));
    return temp.num_ * temp.den_ >= 0;
  };

  friend bool operator<(const BigFraction &lhs, BigInteger second) {
    BigFraction temp(lhs - BigFraction(std::move(second)));
    return temp.num_ * temp.den_ < 0;
  };

  friend bool operator<=(const BigFraction &lhs, BigInteger second) {
    BigFraction temp(lhs - BigFraction(std::move(second)));
    return temp.num_ * temp.den_ <= 0;
  };

  friend bool operator>(BigInteger first, const BigFraction &rhs) {
    BigFraction temp(BigFraction(std::move(first)) - rhs);
    return temp.num_ * temp.den_ > 0;
  };

  friend bool operator>=(BigInteger first, const BigFraction &rhs) {
    BigFraction temp(BigFraction(std::move(first)) - rhs);
    return temp.num_ * temp.den_ >= 0;
  };

  friend bool operator<(BigInteger first, const BigFraction &rhs) {
    BigFraction temp(BigFraction(std::move(first)) - rhs);
    return temp.num_ * temp.den_ < 0;
  };

  friend bool operator<=(BigInteger first, const BigFraction &rhs) {
    BigFraction temp(BigFraction(std::move(first)) - rhs);
    return temp.num_ * temp.den_ <= 0;
  };

  friend bool operator==(const BigFraction &lhs, const BigFraction &rhs) {
    BigFraction temp(lhs - rhs);
    return temp.num_ == 0;
  };

  friend bool operator!=(const BigFraction &lhs, const BigFraction &rhs) {
    BigFraction temp(lhs - rhs);
    return temp.num_ != 0;
  };

  friend bool operator==(const BigFraction &lhs, BigInteger second) {
    BigFraction temp(lhs - BigFraction(std::move(second)));
    return temp.num_ == 0;
  };

  friend bool operator!=(const BigFraction &lhs, BigInteger second) {
    BigFraction temp(lhs - BigFraction(std::move(second)));
    return temp.num_ != 0;
  };

  friend bool operator==(BigInteger first, const BigFraction &rhs) {
    BigFraction temp(BigFraction(std::move(first)) - rhs);
    return temp.num_ == 0;
  };

  friend bool operator!=(BigInteger first, const BigFraction &rhs) {
    BigFraction temp(BigFraction(std::move(first)) - rhs);
    return temp.num_ != 0;
  };

  friend BigFraction operator+=(BigFraction &lhs, const BigFraction &rhs) {
    lhs = lhs + rhs;
    return lhs;
  }

  friend BigFraction operator-=(BigFraction &lhs, const BigFraction &rhs) {
    lhs = lhs - rhs;
    return lhs;
  }

  friend BigFraction operator*=(BigFraction &lhs, const BigFraction &rhs) {
    lhs = lhs * rhs;
    return lhs;
  }

  friend BigFraction operator/=(BigFraction &lhs, const BigFraction &rhs) {
    lhs = lhs / rhs;
    return lhs;
  };

  friend BigFraction operator+=(BigFraction &lhs, BigInteger second) {
    lhs = (lhs + BigFraction(std::move(second)));
    return lhs;
  };

  friend BigFraction operator-=(BigFraction &lhs, BigInteger second) {
    lhs = (lhs - BigFraction(std::move(second)));
    return lhs;
  };

  friend BigFraction operator*=(BigFraction &lhs, BigInteger second) {
    lhs = (lhs * BigFraction(std::move(second)));
    return lhs;
  };

  friend BigFraction operator/=(BigFraction &lhs, BigInteger second) {
    lhs = (lhs / BigFraction(std::move(second)));
    return lhs;
  };

  friend BigFraction operator+(BigFraction hs) {
    if (hs.num_ < 0) hs.num_ = hs.num_ * -1;
    return hs;
  };

  friend BigFraction operator-(BigFraction hs) {
    if (hs.num_ > 0) hs.num_ = hs.num_ * -1;
    return hs;
  };

  friend std::ostream &operator<<(std::ostream &out, const BigFraction &fract) {
    out << fract.num_ << "/" << fract.den_;
    return out;
  }

  BigFraction convert_to_frac(int aim) {
    BigFraction Ans = *this;
    Ans.num_ = Ans.num_.convert_to_bigint(aim);
    Ans.den_ = Ans.den_.convert_to_bigint(aim);
    return Ans;
  };

  std::string convert_to_string_full(int new_base = -1) {

    if (new_base == -1)
      new_base = get_base();
    if (num_ == 0) return "0";
    BigInteger num_in_base_10 = num_.convert_to_bigint(10);
    BigInteger den_in_base_10 = den_.convert_to_bigint(10);

    bool is_negative = false;
    if ((num_in_base_10 < 0) != (den_in_base_10 < 0)) {
      is_negative = true;
    }

    if (num_in_base_10 < 0) num_in_base_10 = -num_in_base_10;
    if (den_in_base_10 < 0) den_in_base_10 = -den_in_base_10;

    BigInteger integer_part = num_in_base_10 / den_in_base_10;
    BigInteger remainder = num_in_base_10 % den_in_base_10;

    std::string result = integer_part.convert_to_string(get_base());

    if (is_negative && !(integer_part == 0)) {
      result = "-" + result;
    }

    if (remainder != 0) {
      result += ".";
      std::vector<std::pair<BigInteger, size_t> > remainder_history;

      std::string fractional_part;

      size_t max_digits = 100000000;
      size_t position = 0;

      while (remainder != 0 && position < max_digits) {
        bool found = false;
        size_t cycle_start = 0;
        for (auto & i : remainder_history) {
          if (i.first == remainder) {
            found = true;
            cycle_start = i.second;
            break;
          }
        }

        if (found) {
          fractional_part.insert(cycle_start, "(");
          fractional_part += ')';
          break;
        }

        remainder_history.emplace_back(remainder, fractional_part.size());

        remainder = remainder * new_base;

        BigInteger digit = remainder / den_in_base_10;
        remainder = remainder % den_in_base_10;

        fractional_part += digit.convert_to_string(new_base);

        position++;
      }

      result += fractional_part;
    }

    return result;
  }

  std::string convert_to_string(int new_base = -1) {
    if (new_base == -1)
      new_base = get_base();

    if (num_ == 0) return "0";
    BigInteger num_in_base_10 = num_.convert_to_bigint(10);
    BigInteger den_in_base_10 = den_.convert_to_bigint(10);

    bool is_negative = false;
    if ((num_in_base_10 < 0) != (den_in_base_10 < 0)) {
      is_negative = true;
    }

    if (num_in_base_10 < 0) num_in_base_10 = -num_in_base_10;
    if (den_in_base_10 < 0) den_in_base_10 = -den_in_base_10;

    BigInteger integer_part = num_in_base_10 / den_in_base_10;
    BigInteger remainder = num_in_base_10 % den_in_base_10;

    std::string result;

    if (is_negative && !(integer_part == 0)) {
      result = "-" + result;
    }

    if (remainder != 0) {

      std::vector<std::pair<BigInteger, size_t> > remainder_history;

      std::string fractional_part;

      size_t max_digits = 10000;
      size_t position = 0;

      while (remainder != 0 && position < max_digits) {
        bool found = false;
        size_t cycle_start = 0;
        for (auto & i : remainder_history) {
          if (i.first == remainder) {
            found = true;
            cycle_start = i.second;
            break;
          }
        }

        if (found) {
          fractional_part.insert(cycle_start, "(");
          fractional_part += ')';
          break;
        }

        remainder_history.emplace_back(remainder, fractional_part.size());

        remainder = remainder * new_base;

        BigInteger digit = remainder / den_in_base_10;
        remainder = remainder % den_in_base_10;

        fractional_part += digit.convert_to_string(new_base);

        position++;
      }

      result += fractional_part;
    }

    return result;
  }
   int get_base(){
    return num_.get_base();
  }



 private:
    BigInteger num_, den_;
    int base = num_.get_base();
};

BigInteger pow(BigInteger &base, int exponent) {
  if (exponent < 0)
    throw std::invalid_argument("Отрицательные степени не поддерживаются.");

  BigInteger result(1, base.get_base());
  BigInteger base_copy = base;
  int exp = exponent;

  while (exp > 0) {
    if (exp % 2 == 1) {
      result = result * base_copy;
    }
    base_copy = base_copy * base_copy;
    exp /= 2;
  }

  return result;
}

class Period {
 private:
  BigInteger per;
  std::string period_digits;
  int base;
  int zeros = 0;

 public:
  Period() {
    period_digits = {};
    base = 10;
  }
  Period(std::string periodic_str, int base_now, int zero_before = 0)
      : period_digits(std::move(periodic_str)), base(base_now), zeros(zero_before) {
  }

  Period(BigInteger per_, int base_now, int zero_before = 0)
      : per(per_), base(base_now), zeros(zero_before) {
  }


  std::pair<BigInteger, BigInteger> to_fraction_per() const {
    std::string zeros_box;
    for (int i = 0; i < zeros; ++i) zeros_box += '0';
    BigInteger base_bigint(base, base);
    BigInteger denominator = pow(base_bigint,  get_len(per) - 1) * per;
    std::cout << "pizda: " << denominator << std::endl;
    std::cout << "xui: " << get_len(per) << std::endl;
    denominator = {denominator.convert_to_string(base) + zeros_box, base};
    std::cout << "z1: " << per << " " << denominator << std::endl << std::endl;
    return {per, denominator};
  }


  std::pair<BigInteger, BigInteger> to_fraction() const {
    std::string temopator;
    for (int i = 0; i < zeros; ++i) temopator += '0';
    BigInteger numerator(period_digits, base);
    BigInteger base_bigint(base, base);
    BigInteger denominator = pow(base_bigint, get_len(numerator)) - BigInteger(1, base);
    BigInteger xui(denominator.convert_to_string(base) + temopator, base);
    return {numerator, xui};
  }

  [[nodiscard]] std::string get_period_digits() const {
    return period_digits;
  }

  [[nodiscard]] int get_base() const {
    return base;
  }
};

class BigNum {
 private:
  BigInteger integer_part;
  BigFraction fractional_part;
  Period period_part;
  BigFraction true_period_part;
  BigFraction total_frac;
  BigFraction total_num;
  bool state_frac = true, state_per = true;
  int base;

 public:
  BigNum(const std::string &num_str, int base_now) : base(base_now) {
    std::string int_part_str;
    std::string frac_part_str;
    std::string period_str;
    int state = 0;

    for (size_t i = 0; i < num_str.length(); ++i) {
      char c = num_str[i];
      if (c == '.') {
        state = 1;
      } else if (c == '(') {
        if (state == 0) throw std::invalid_argument("Period in integer");
        else state = 2;
      } else if (c == ')') {
        break;
      } else {
        if (state == 0) {
          int_part_str += c;
        } else if (state == 1) {
          frac_part_str += c;
        } else {
          period_str += c;
        }
      }

    }
    integer_part = {int_part_str, base_now};

    if (!frac_part_str.empty()) {
      fractional_part = BigFraction(frac_part_str, base_now);
    } else {
      state_frac = false;
      fractional_part = {"0", base_now};
    }

    if (!period_str.empty()) {
      int frac_len = 0, st = 1;
      for (auto sym : frac_part_str){
        if (sym == '['){
          st = 0;
          frac_len+=1;
        } else if (sym == ']'){
          st = 1;
        } else if (!st){
          continue;
        }else {
          frac_len+=1;
        }
      }

        period_part = {period_str, base_now, frac_len};
        std::pair<BigInteger, BigInteger> ans_period_fraction = period_part.to_fraction();
        true_period_part = BigFraction(ans_period_fraction.first, ans_period_fraction.second);


    } else {
      state_per = false;
      true_period_part = {"0", base_now};
    }

    total_frac = true_period_part + fractional_part;
    total_num = total_frac + BigFraction(integer_part, BigInteger(1, base_now));


  }


  friend BigFraction operator+(const  BigNum&lhs, const BigNum &rhs) {
    return (lhs.total_num + rhs.total_num);
  };

  friend BigFraction operator-(const  BigNum&lhs, const BigNum &rhs) {
    return (lhs.total_num - rhs.total_num);
  };

  friend BigFraction operator*(const  BigNum&lhs, const BigNum &rhs) {
    return (lhs.total_num * rhs.total_num);
  };

  friend BigFraction operator/(const  BigNum&lhs, const BigNum &rhs) {
    return (lhs.total_num / rhs.total_num);
  };

  std::string convert_to_string(int base_new) {
    std::string ans_integer = integer_part.convert_to_string(base_new);
    std::string ans_final = ans_integer;
    if (state_per or state_frac) {
      ans_final += '.';
      if (state_per and state_frac){

        ans_final+=total_frac.convert_to_string(base_new);
      } else{
        if (state_frac) {
          ans_final += fractional_part.convert_to_string(base_new);
        }else {

          std::string ans_period = total_frac.convert_to_string(base_new);
          ans_final += ans_period;
        }
      }
      if (ans_final[ans_final.size() - 1] == '.') {
        BigInteger up = integer_part + 1;
        ans_final = up.convert_to_string(base_new);
        ans_final+= ".0";
      }

    }
    return ans_final;
  }
};

void i_hate_practise() {
  int server_fd, new_socket;
  struct sockaddr_in address{};
  int opt = 1;
  int addr_len = sizeof(address);
  char buffer[200000] = {0};

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    std::cerr << "Ошибка при создании сокета" << std::endl;
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    std::cerr << "Ошибка при установке опций сокета" << std::endl;
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(65432);

  if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
    std::cerr << "Ошибка при привязке сокета" << std::endl;
  }

  if (listen(server_fd, 3) < 0) {
    std::cerr << "Ошибка при прослушивании" << std::endl;
  }

  std::cout << "Ожидание подключения..." << std::endl;

  while (true) {
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addr_len)) < 0) {
      std::cerr << "Ошибка при принятии соединения" << std::endl;
      continue;
    }

    int val_read = static_cast<int>((read(new_socket, buffer, 1024)));
    std::string received_data(buffer, val_read);

    std::string type, float_value, int_value1, int_value2, oper;
    size_t pos = 0;
    int index = 0;

    while ((pos = received_data.find(',')) != std::string::npos) {
      if (index == 0) {
        type = received_data.substr(0, pos);
      } else if (index == 1) {
        float_value = received_data.substr(0, pos);
      } else if (index == 2) {
        int_value1 = received_data.substr(0, pos);
      }
      if (type == "arif"){
        if (index == 3){
          oper = received_data.substr(0, pos);
        }
      }

      received_data.erase(0, pos + 1);
      index++;
    }
    int_value2 = received_data;
    std::string response;

    if (type == "convert"){

      std::cout << "Полученные данные:" << std::endl;
      std::cout << "Число для перевода: " << float_value << std::endl;
      std::cout << "Текущая система счисления: " << int_value1 << std::endl;
      std::cout << "Целевая система счисления: " << int_value2 << std::endl;


      try {
        int current_base = std::stoi(int_value1);
        int target_base = std::stoi(int_value2);

        BigNum inputNumber(float_value, current_base);

        response = inputNumber.convert_to_string(target_base);
      } catch (const std::invalid_argument &e) {
        response = "Ошибка: Некорректные входные данные. " + std::string(e.what());
        std::cerr << response << std::endl;
      } catch (const std::out_of_range &e) {
        response = "Ошибка: Входные данные выходят за допустимые границы. " + std::string(e.what());
        std::cerr << response << std::endl;
      } catch (const std::exception &e) {
        response = "Ошибка: Непредвиденная ошибка. " + std::string(e.what());
        std::cerr << response << std::endl;
      } catch (...) {
        response = "Ошибка: Неизвестная ошибка.";
        std::cerr << response << std::endl;
      }



    } else if (type == "arif"){
      std::swap(int_value2, oper);

      int current_base = std::stoi(int_value2);
      BigNum firs = {static_cast<std::string>(float_value), current_base};
      BigNum ces = {static_cast<std::string>(int_value1), current_base};

      if (oper == "+") response = BigFraction(firs + ces).convert_to_string_full();
      if (oper == "-") response = BigFraction(firs - ces).convert_to_string_full();
      if (oper == "*") response = BigFraction(firs * ces).convert_to_string_full();
      if (oper == "/") response = BigFraction(firs / ces).convert_to_string_full();

    }else{
      response = "Ошибка: NoType";
    }
    std::cout << std::endl << response << std::endl;
    send(new_socket, response.c_str(), response.size(), 0);

    close(new_socket);
  }
}

int main() {
  i_hate_practise();
  std::string float_value = "0.8(00001112222221321411111118306)";
  int current_base = 12, target_base =  10;

  BigNum inputNumber(float_value, current_base);

  std::string response = inputNumber.convert_to_string(target_base);
  std::cout << response;
}