// 原作者 hqztrue，Maigo 进行了修改
//
// adding MPI + openMP parallelization
//  - compile with IntelMPI + icpc : mpiicpc -fopenmp expressions_count_mpi_openmp.cpp -o expressions_count_mpi_openmp
//  - run with command `mpirun ./expressions_count_mpi_openmp n_variables save_level`
//  - save_level controls the parallelization, save_level = 3 seems to be a good choice.
// Yi Yao
// results for n_variables 10 and 11:
// [ FINAL ] Found 2894532443154 expressions, 88768176256 neutral, 1402882133449 positive, 1402882133449 negative
// [ FINAL ] Found 171800282010062 expressions, 3797784543104 neutral, 84001248733479 positive, 84001248733479 negative

#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <assert.h>
#include <omp.h>
#include <mpi.h>
using namespace std::chrono;
using namespace std;

// 算式总数，以及无、正、负极性算式数目
//long long total, neutral, pos, neg;

// 结点类
struct Node {
  char ch;
  Node *l, *r;
  int polar, id;
  Node(char _ch = 0, Node* _l = nullptr, Node* _r = nullptr, int _polar = 0, int _id = 0)
      : ch(_ch), l(_l), r(_r), polar(_polar), id(_id) {}
  string to_string() const;
  //Node* copy() const;
};

Node* copy(Node* root){
    if(root == nullptr){return nullptr;}
    else{
        Node* temp = new Node(root->ch, nullptr, nullptr, root->polar, root->id);
        temp -> l = copy(root -> l);
        temp -> r = copy(root -> r);
        return temp;
        }
}

string Node::to_string() const {
  string str;
  if (!(this->ch == '+' || this->ch == '-' || this->ch == '*' || this->ch == '/')) {
    str += this->ch;
    return str;
  }
  string left = this->l->to_string();
  string right = this->r->to_string();
  if ((this->ch == '*' || this->ch == '/') && (this->l->ch == '+' || this->l->ch == '-')) {
    left = '(' + left + ')';
  }
  if (((this->ch == '/') && (this->r->ch == '+' || this->r->ch == '-' || this->r->ch == '*' || this->r->ch == '/')) ||
      ((this->ch == '*' || this->ch == '-') && (this->r->ch == '+' || this->r->ch == '-'))) {
        right = '(' + right + ')';
      }
  return left + ' ' + this->ch + ' ' + right;
}

std::ostream &operator<<(std::ostream &os, Node const &n) { 
    return os << n.to_string();
}

vector<pair<vector<Node>, int>> trees_bases;

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

// 枚举由 n 个变量组成的算式 for base_trees generation
void DFS(vector<Node>& trees, int minj, int saveLevel) {                           // trees 为当前算式列表，minj 为第二棵子树的最小下标
  if (saveLevel == 0) {
    vector<Node> trees_copy;
    for (Node n: trees) trees_copy.push_back(*copy(&n));
    trees_bases.push_back({trees_copy, minj});
    return;
  }
  
  saveLevel--;

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
        DFS(trees, j - 1, saveLevel);                                          // 递归下去
        trees.pop_back();                                           // 从列表中去掉运算结果
      }
      trees.insert(trees.begin() + i, left);                        // 把第一棵子树加回列表
    }
    trees.insert(trees.begin() + j, right);                         // 把第二棵子树加回列表
  }
}

// 枚举由 n 个变量组成的算式 for enumeration
void DFS(vector<Node>& trees, int minj, long long& total, long long& neutral, long long& pos, long long& neg) {                           // trees 为当前算式列表，minj 为第二棵子树的最小下标
  if (trees.size() == 1) {                                          // 只剩一个算式
    //cout << trees[0] << endl;
    ++total;                                                        // 计入所有算式总数
    switch (trees[0].polar) {                                       // 计入相应极性算式的总数
      case 1: ++pos; break;
      case 0: ++neutral; break;
      case -1: ++neg; break;
    }
    if (total % 1000000 == 0) {                                     // 告诉用户：我还活着！
      cout << "OMP thread: " << omp_get_thread_num() << " [Partial] Found " << total << " expressions, " << neutral << " neutral, "
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
        DFS(trees, j - 1, total, neutral, pos, neg);                                          // 递归下去
        trees.pop_back();                                           // 从列表中去掉运算结果
      }
      trees.insert(trees.begin() + i, left);                        // 把第一棵子树加回列表
    }
    trees.insert(trees.begin() + j, right);                         // 把第二棵子树加回列表
  }
}

// 主程序
int main(int argc, char* argv[]) {
  int mpi_ierr;
  mpi_ierr = MPI_Init ( &argc, &argv );
  auto start = high_resolution_clock::now();
  assert("two input numbers! 1. number of variables, 2. save level" && (argc == 3));
  int mpi_id;
  int mpi_p;
  mpi_ierr = MPI_Comm_size ( MPI_COMM_WORLD, &mpi_p );
  mpi_ierr = MPI_Comm_rank ( MPI_COMM_WORLD, &mpi_id );
  cout << "mpi_p, mpi_id:" << mpi_p << " " << mpi_id << endl;
  int n = atoi(argv[1]);                                            // 输入变量个数
  int saveLevel = atoi(argv[2]);
  assert("We need save level < n" && (saveLevel < n));
  vector<Node> trees;
  for (int i = 0; i < n; ++i) {
    trees.push_back(Node('a' + i, nullptr, nullptr, 0, i));                     // 生成 n 个由单变量组成的初始算式
  }
  DFS(trees, 1, saveLevel);                                                    // 枚举算式并统计总数
  long long total = 0;
  long long neutral = 0;
  long long pos = 0;
  long long neg = 0;
  long long total_mpi = 0;
  long long neutral_mpi = 0;
  long long pos_mpi = 0;
  long long neg_mpi = 0;
  #pragma omp parallel 
  {
    if (omp_get_thread_num() == 0) {
      cout << "Number of OMP threads: " << omp_get_num_threads() << endl;
      cout << "Solving " << trees_bases.size() << " subtrees" << endl;
    }
    #pragma omp for schedule(dynamic) reduction(+:total_mpi, neutral_mpi, pos_mpi, neg_mpi)
    for (int i = mpi_id; i < trees_bases.size(); i+=mpi_p) {
      auto p = trees_bases[i];
      auto tmptrees = p.first;
      int minj = p.second;
      DFS(tmptrees, minj, total_mpi, neutral_mpi, pos_mpi, neg_mpi);
    }
  }
  mpi_ierr = MPI_Reduce ( &total_mpi, &total, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD );
  mpi_ierr = MPI_Reduce ( &neutral_mpi, &neutral, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD );
  mpi_ierr = MPI_Reduce ( &pos_mpi, &pos, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD );
  mpi_ierr = MPI_Reduce ( &neg_mpi, &neg, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD );
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  if (mpi_id == 0) {
    cout << "[ FINAL ] Found " << total << " expressions, " << neutral << " neutral, "
         << pos << " positive, " << neg << " negative" << endl;
    cout << "Time taken by function: "
       << double(duration.count()) / 1e6 << " seconds" << endl;
  }
  MPI_Finalize ( );

  return 0;
}