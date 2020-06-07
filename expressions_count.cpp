// 原作者 hqztrue，Maigo 进行了修改

#include <iostream>
#include <vector>
using namespace std;

// 算式总数，以及无、正、负极性算式数目
long long total, neutral, pos, neg;

// 结点类
struct Node {
  char ch;
  Node *l, *r;
  int polar, id;
  Node(char _ch = 0, Node* _l = 0, Node* _r = 0, int _polar = 0, int _id = 0)
      : ch(_ch), l(_l), r(_r), polar(_polar), id(_id) {}
};

// 用加减乘除结合两个算式，去重
vector<Node> actions(Node& l, Node& r) {
  vector<Node> res;
  // 加法：两个孩子都不能是减号；左孩子还不能是加号；
  //       若右孩子是加号，则左孩子和右孩子的左孩子要满足单调性
  if (l.ch != '+' && l.ch != '-' && r.ch != '-' && (r.ch != '+' || l.id < r.l->id)) {
    if (l.polar == 0 || r.polar == 0) {
      res.push_back(Node('+', &l, &r, l.polar + r.polar));          // 无极性 + 无极性 = 无极性
                                                                    // 有极性 + 无极性 = 有极性者的极性
    }
    else {
      res.push_back(Node('+', &l, &r, r.polar));                    // 有极性 + 有极性 = 右子树极性
    }
  }
  // 减法：两个孩子都不能是减号
  if (l.ch != '-' && r.ch != '-') {
    if (l.polar == 0 && r.polar == 0) {                             // 无极性 - 无极性：
      res.push_back(Node('-', &l, &r, 1));                          // 正过来减是正极性
      res.push_back(Node('-', &r, &l, -1));                         // 反过来减是负极性
    }
    else {
      if (l.polar == 0) res.push_back(Node('-', &r, &l, r.polar));  // 有极性 - 无极性 = 有极性者的极性
                                                                    // （无极性 - 有极性 = 舍弃）
                                                                    // （有极性 - 有极性 = 舍弃）
      if (r.polar == 0) res.push_back(Node('-', &l, &r, l.polar));  // 同上
    }
  }
  // 乘法：两个孩子都不能是除号；左孩子还不能是乘号；
  //       若右孩子是乘号，则左孩子和右孩子的左孩子要满足单调性
  if (l.ch != '*' && l.ch != '/' && r.ch != '/' && (r.ch != '*' || l.id < r.l->id)) {
    if (l.polar == 0 || r.polar == 0) {
      res.push_back(Node('*', &l, &r, l.polar + r.polar));          // 无极性 * 无极性 = 无极性
                                                                    // 有极性 * 无极性 = 有极性者的极性
    }
    else if (l.polar > 0) {
      res.push_back(Node('*', &l, &r, r.polar));                    // 正极性 * 有极性 = 右子树极性
                                                                    // （负极性 * 有极性 = 舍弃）
    }
  }
  // 除法：两个孩子都不能是除号
  if (l.ch != '/' && r.ch != '/') {
    if (l.polar == 0 || r.polar == 0) {
      res.push_back(Node('/', &l, &r, l.polar + r.polar));          // 无极性 / 无极性 = 无极性
                                                                    // 有极性 / 无极性 = 有极性者的极性
                                                                    // 无极性 / 有极性 = 有极性者的极性
      res.push_back(Node('/', &r, &l, l.polar + r.polar));          // 同上
    }
    else {
      if (l.polar > 0) res.push_back(Node('/', &l, &r, r.polar));   // 正极性 / 有极性 = 右子树极性
                                                                    // （负极性 / 有极性 = 舍弃）
      if (r.polar > 0) res.push_back(Node('/', &r, &l, l.polar));   // 同上
    }
  }
  return res;
}

// 枚举由 n 个变量组成的算式
void DFS(vector<Node>& trees, int minj) {                           // trees 为当前算式列表，minj 为第二棵子树的最小下标
  if (trees.size() == 1) {                                          // 只剩一个算式
    ++total;                                                        // 计入所有算式总数
    switch (trees[0].polar) {                                       // 计入相应极性算式的总数
      case 1: ++pos; break;
      case 0: ++neutral; break;
      case -1: ++neg; break;
    }
    if (total % 1000000 == 0) {                                     // 告诉用户：我还活着！
      cout << "[Partial] Found " << total << " expressions, " << neutral << " neutral, "
           << pos << " positive, " << neg << " negative" << endl;
    }
    return;
  }

  int new_id = trees.back().id + 1;                                 // 新结点 id
  for (int j = minj; j < trees.size(); ++j) {                       // 枚举第二棵子树
    Node right = trees[j];
    trees.erase(trees.begin() + j);                                 // 从列表中去掉第二棵子树
    for (int i = 0; i < j; ++i) {                                   // 枚举第一棵子树
      Node left = trees[i];
      trees.erase(trees.begin() + i);                               // 从列表中去掉第一棵子树
      for (Node result : actions(left, right)) {                    // 枚举运算符
        result.id = new_id;                                         // 为新结点赋予 id
        trees.push_back(result);                                    // 往列表中加入运算结果
        DFS(trees, j - 1);                                          // 递归下去
        trees.pop_back();                                           // 从列表中去掉运算结果
      }
      trees.insert(trees.begin() + i, left);                        // 把第一棵子树加回列表
    }
    trees.insert(trees.begin() + j, right);                         // 把第二棵子树加回列表
  }
}

// 主程序
int main(int argc, char* argv[]) {
  int n = atoi(argv[1]);                                            // 输入变量个数
  vector<Node> trees;
  for (int i = 0; i < n; ++i) {
    trees.push_back(Node('a' + i, 0, 0, 0, i));                     // 生成 n 个由单变量组成的初始算式
  }
  DFS(trees, 1);                                                    // 枚举算式并统计总数
  cout << "[ FINAL ] Found " << total << " expressions, " << neutral << " neutral, "
       << pos << " positive, " << neg << " negative" << endl;
  return 0;
}
