#include "ModParser.h"
#include "CmodError.h"
#include <cctype>
#include <limits>

static CmodError invalidModExpression(const std::string& message) {
  return CmodError(CmodError::Kind::Project, message,
                   "Sieve -> MODS expression",
                   "Use positive integer moduli, matching parentheses, and U, I, or ~ operators. Provide an offset for each modulus.");
}

ModParser::Token::Token(int n, int minVal, int maxVal, int offset)
                       : n(n)
                       , minVal(minVal)
                       , maxVal(maxVal)
                       , offset(offset)
                       , hasList(false) {}

ModParser::Token::Token(std::list<int> l)
                       : l(l)
                       , hasList(true) {}

//----------------------------------------------------------------------------//

const std::list<int>& ModParser::Token::getList() {
  if (hasList) return l;
  hasList = true;
  l = modList(n, minVal, maxVal, offset);

  return l;
}


//----------------------------------------------------------------------------//

int ModParser::Token::getInt() {
  return n;
}


//----------------------------------------------------------------------------//

std::list<int> ModParser::modList(int mod, int min, int max, int offset) {
  std::list<int> result;
  long long startNum = static_cast<long long>(min) + offset + (mod - (min % mod)) % mod;
  startNum -= mod * ((startNum - min)/mod);
       
  for (long long i = startNum; i <= max; i += mod) {
    result.push_back(static_cast<int>(i));
  }

  return result;
}


//----------------------------------------------------------------------------//

std::list<int> ModParser::offsetList(const std::list<int>& l, int offset) {
  std::list<int> result;
  for (std::list<int>::const_iterator it = l.begin(); it != l.end(); ++it) {
    result.push_back(*it + offset);
  }
  return result;
}


//----------------------------------------------------------------------------//

std::list<int> ModParser::listUnion(const std::list<int>& a, const std::list<int>& b) {
  std::list<int> result(a.size() + b.size());
  std::list<int>::iterator it = set_union(a.begin(), a.end(), b.begin(), b.end(), result.begin());
  result.erase(it, result.end());
  return result;
}


//----------------------------------------------------------------------------//

std::list<int> ModParser::listIntersection(const std::list<int>& a, const std::list<int>& b) {
  std::list<int> result(std::min(a.size(), b.size()));
  std::list<int>::iterator it = set_intersection(a.begin(), a.end(), b.begin(), b.end(), result.begin());
  result.erase(it, result.end());
  return result;
}


//----------------------------------------------------------------------------//

std::list<int> ModParser::listDifference(const std::list<int>& a, const std::list<int>& b) {
  std::list<int> result(a.size());
  std::list<int>::iterator it = set_difference(a.begin(), a.end(), b.begin(), b.end(), result.begin());
  result.erase(it, result.end());
  return result;
}


//----------------------------------------------------------------------------//

std::list<int> ModParser::listComplement(const std::list<int>& l, int minVal, int maxVal) {
  std::list<int> result;
  int elem = minVal;
  for (std::list<int>::const_iterator it = l.begin(); it != l.end() && elem <= *it && elem <= maxVal; ++it) {
    while (elem < *it && elem <= maxVal) {
      result.push_back(elem);
      ++elem;
    }
    ++elem;
  }
  return result;
}


//----------------------------------------------------------------------------//

void ModParser::parseOperator(std::vector<ModParser::Token>& operands, std::stack<char>& operators, int minVal, int maxVal) {
  char op = operators.top();
  operators.pop();
  std::list<int> result;
  const size_t required = op == '~' ? 1 : 2;
  if (operands.size() < required) {
    throw invalidModExpression("MODS operator '" + std::string(1, op) + "' is missing an operand.");
  }

  if (op == '~') {
    result = listComplement(operands.back().getList(), minVal, maxVal);
    operands.pop_back();
    operands.push_back(result);
    return;
  }

  Token& b = operands.back();
  Token& a = operands[operands.size() - 2];

  switch (op) {
    case 'U':
      result = listUnion(a.getList(), b.getList());
      break;
    case 'I':
      result = listIntersection(a.getList(), b.getList());
      break;
    case '-':
      result = listDifference(a.getList(), b.getList());
      break;
  }

  operands.pop_back();
  operands.pop_back();
  operands.push_back(result);
}


//----------------------------------------------------------------------------//

int ModParser::precedence(char c) {
  switch (c) {
    case 'U': return 1;
    case 'I': return 50;
    case '~': return 99;
    default: return -1;
  }
}


//----------------------------------------------------------------------------//

void ModParser::parseExpr(const std::string& exp, int minVal, int maxVal) {
  std::vector<ModParser::Token> operands;
  std::stack<char> operators;
  unsigned chNum = 0;
  size_t modIndex = 0;
  bool needsOperand = true;
  while (chNum < exp.size()) {
    char ch = exp[chNum];
    if (std::isdigit(static_cast<unsigned char>(ch))) {
      if (!needsOperand) {
        throw invalidModExpression("MODS expression is missing an operator before position " + std::to_string(chNum + 1) + ".");
      }
      int num = 0;
      while (chNum < exp.size() && std::isdigit(static_cast<unsigned char>(exp[chNum]))) {
        ch = exp[chNum];
        if (num > (std::numeric_limits<int>::max() - (ch - '0')) / 10) {
          throw invalidModExpression("MODS modulus exceeds the supported integer range.");
        }
        num *= 10;
        num += ch - '0';
        ++chNum;
      }
      if (num == 0) {
        throw invalidModExpression("MODS modulus 0 would cause division by zero.");
      }
      if (modIndex >= _offsets.size()) {
        throw invalidModExpression("MODS modulus " + std::to_string(modIndex + 1) + " has no matching offset.");
      }
      operands.push_back(Token(num, minVal, maxVal, _offsets[modIndex]));
      _mods.push_back(num);
      ++modIndex;
      needsOperand = false;
    } else if (precedence(ch) != -1) {
      if ((ch == '~') != needsOperand) {
        throw invalidModExpression("MODS operator '" + std::string(1, ch) + "' is misplaced or missing an operand.");
      }
      while (ch != '~' && !operators.empty()
             && precedence(operators.top()) != -1
             && precedence(operators.top()) >= precedence(ch)) {
        parseOperator(operands, operators, minVal, maxVal);
      }
      operators.push(ch);
      ++chNum;
      needsOperand = true;
    } else if (ch == '(') {
      if (!needsOperand) {
        throw invalidModExpression("MODS expression is missing an operator before an opening parenthesis.");
      }
      operators.push('(');
      ++chNum;
    } else if (ch == ')') {
      if (needsOperand) {
        throw invalidModExpression("MODS closing parenthesis has no preceding operand.");
      }
      while (!operators.empty() && operators.top() != '(') {
        parseOperator(operands, operators, minVal, maxVal);
      }
      if (operators.empty()) {
        throw invalidModExpression("MODS closing parenthesis has no matching opening parenthesis.");
      }
      operators.pop();
      ++chNum;
    } else {
      if (!std::isspace(static_cast<unsigned char>(ch))) {
        throw invalidModExpression("MODS expression contains unsupported character '" + std::string(1, ch) + "'.");
      }
      ++chNum;
    }
  }
  if (needsOperand) {
    throw invalidModExpression("MODS expression '" + exp + "' is empty or ends without an operand.");
  }
  while (!operators.empty()) {
    if (operators.top() == '(') {
      throw invalidModExpression("MODS opening parenthesis has no matching closing parenthesis.");
    }
    parseOperator(operands, operators, minVal, maxVal);
  }
  if (operands.size() != 1) {
    throw invalidModExpression("MODS expression does not reduce to one sieve.");
  }
  _elements = operands.back().getList();
}


//----------------------------------------------------------------------------//

ModParser::ModParser(std::vector<int> offsets) : _offsets(offsets) {
}
