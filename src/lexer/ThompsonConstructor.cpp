#include <metalyzer/lexer/NFA.hpp>
#include <metalyzer/lexer/ThompsonConstructor.hpp>

#include <stack>
#include <string>

namespace metalyzer {

NFA ThompsonConstructor::createBasic(char symbol) {
  NFAState *q0 = ctx.createState();
  NFAState *qf = ctx.createState();

  q0->transitions[symbol].push_back(qf);

  NFA basic_nfa;
  basic_nfa.start = q0;
  basic_nfa.end = qf;

  basic_nfa.allStates.push_back(q0);
  basic_nfa.allStates.push_back(qf);

  return basic_nfa;
};

NFA ThompsonConstructor::createUnion(NFA left, NFA right) {
  NFAState *q0 = ctx.createState();
  NFAState *qf = ctx.createState();

  q0->transitions['\0'].push_back(left.start);
  q0->transitions['\0'].push_back(right.start);

  left.end->transitions['\0'].push_back(qf);
  right.end->transitions['\0'].push_back(qf);

  NFA union_nfa;

  union_nfa.start = q0;
  union_nfa.end = qf;

  union_nfa.allStates.push_back(q0);
  union_nfa.allStates.insert(union_nfa.allStates.end(), left.allStates.begin(),
                             left.allStates.end());
  union_nfa.allStates.insert(union_nfa.allStates.end(), right.allStates.begin(),
                             right.allStates.end());
  union_nfa.allStates.push_back(qf);

  return union_nfa;
};

NFA ThompsonConstructor::createConcat(NFA left, NFA right) {
  left.end->transitions['\0'].push_back(right.start);

  NFA concat_nfa;

  concat_nfa.start = left.start;
  concat_nfa.end = right.end;

  concat_nfa.allStates.insert(concat_nfa.allStates.end(),
                              left.allStates.begin(), left.allStates.end());
  concat_nfa.allStates.insert(concat_nfa.allStates.end(),
                              right.allStates.begin(), right.allStates.end());

  return concat_nfa;
};

NFA ThompsonConstructor::createStar(NFA base) {
  NFAState *q0 = ctx.createState();
  NFAState *qf = ctx.createState();

  q0->transitions['\0'].push_back(base.start);
  q0->transitions['\0'].push_back(qf);

  base.end->transitions['\0'].push_back(base.start);
  base.end->transitions['\0'].push_back(qf);

  NFA star_nfa;
  star_nfa.start = q0;
  star_nfa.end = qf;

  star_nfa.allStates.push_back(q0);
  star_nfa.allStates.insert(star_nfa.allStates.end(), base.allStates.begin(),
                            base.allStates.end());
  star_nfa.allStates.push_back(qf);

  return star_nfa;
};

std::string ThompsonConstructor::preprocess(const std::string &regex) {
  std::string res;
  for (int i = 0; i < static_cast<int>(regex.size()); i++) {
    char a = regex[i];
    res += a;
    bool aIsFinisher = (isalnum(a) || a == '*' || a == ')');
    if (i + 1 < static_cast<int>(regex.size())) {
      char b = regex[i + 1];

      bool bIsStarter = (isalnum(b) || b == '(');

      if (aIsFinisher && bIsStarter) {
        res += '.';
      }
    }
  }
  return res;
}

std::string ThompsonConstructor::shunt_yard(const std::string &regex) {
  std::string postfix;
  std::stack<char> st;
  for (char ch : regex) {
    if (isalnum(ch))
      postfix += ch;
    else if (ch == '(')
      st.push(ch);
    else if (ch == ')') {
      while (!st.empty() && st.top() != '(') {
        postfix += st.top();
        st.pop();
      }
      st.pop();
    } else {
      while (!st.empty() && getPrecedence(st.top()) >= getPrecedence(ch)) {
        postfix += st.top();
        st.pop();
      }
      st.push(ch);
    }
  }

  while (!st.empty()) {
    postfix += st.top();
    st.pop();
  }
  return postfix;
}

int ThompsonConstructor::getPrecedence(char op) {
  if (op == '*')
    return 3;
  else if (op == '.')
    return 2;
  else if (op == '|')
    return 1;
  else
    return 0;
}

NFA ThompsonConstructor::build(const std::string &regexPattern) {
  std::string regex = shunt_yard(preprocess(regexPattern));
  std::stack<NFA> eval;
  for (char ch : regex) {
    if (isalnum(ch))
      eval.push(createBasic(ch));
    else if (ch == '*') {
      if (eval.empty())
        continue;
      NFA base = eval.top();
      eval.pop();
      eval.push(createStar(base));
    } else if (ch == '|' || ch == '.') {
      if (eval.size() < 2)
        continue;

      NFA right = eval.top();
      eval.pop();
      NFA left = eval.top();
      eval.pop();

      if (ch == '|')
        eval.push(createUnion(left, right));
      else
        eval.push(createConcat(left, right));
    }
  }

  if (eval.empty()) {
    return NFA{};
  }

  NFA finalNFA = eval.top();

  if (finalNFA.end != nullptr) {
    finalNFA.end->isAccepting = true;
  }

  return finalNFA;
};

} // namespace metalyzer
