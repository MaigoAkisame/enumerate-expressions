# -*- coding: utf-8 -*-

# 结点类
from fractions import Fraction


class Node:
    def __init__(self, ch=None, left=None, right=None, polar=0, id=0):
        self.ch = ch                                    # 变量或运算符
        self.left = left                                # 左孩子
        self.right = right                              # 右孩子
        self.polar = polar                              # 极性，可取 0, 1, -1
        self.id = id                                    # 结点编号

    def __str__(self):                                  # 把树转换成算式
        if self.ch not in '+-*/':
            return self.ch                              # 单变量不加括号
        left = str(self.left)                           # 左子树转字符串
        right = str(self.right)                         # 右子树转字符串
        if self.ch in '*/' and self.left.ch in '+-':
            left = '(' + left + ')'                     # 左子树加括号
        if self.ch == '/' and self.right.ch in '+-*/' or self.ch in '*-' and self.right.ch in '+-':
            right = '(' + right + ')'                   # 右子树加括号
        return left + ' ' + self.ch + ' ' + right       # 用根结点的运算符相连

# 下面是几种 actions 函数，它们接收两个结点，返回它们的各种运算结果
# 「去重」者可以避免交换律、结合律、去括号、反转减号四种原因造成的重复

# 仅考虑加法与乘法，不去重


def naive2(left, right):
    for op in '+*':
        yield Node(op, left, right)
        yield Node(op, right, left)

# 仅考虑加法与乘法，去重


def smart2(left, right):
    for op in '+*':
        if op != left.ch and (op != right.ch or left.id < right.left.id):
            yield Node(op, left, right)

# 考虑加减乘除，不去重


def naive4(left, right):
    for op in '+-*/':
        yield Node(op, left, right)
        yield Node(op, right, left)

# 考虑加减乘除，去重


def smart4(left, right):
    # 加法：两个孩子都不能是减号；左孩子还不能是加号；
    #       若右孩子是加号，则左孩子和右孩子的左孩子要满足单调性
    if left.ch not in '+-' and right.ch != '-' and (right.ch != '+' or left.id < right.left.id):
        if left.polar == 0 or right.polar == 0:
            # 无极性 + 无极性 = 无极性
            yield Node('+', left, right, left.polar + right.polar)
            # 有极性 + 无极性 = 有极性者的极性
        else:
            # 有极性 + 有极性 = 右子树极性
            yield Node('+', left, right, right.polar)
    # 减法：两个孩子都不能是减号
    if left.ch != '-' and right.ch != '-':
        if left.polar == 0 and right.polar == 0:                    # 无极性 - 无极性：
            yield Node('-', left, right, 1)                         # 正过来减是正极性
            yield Node('-', right, left, -1)                        # 反过来减是负极性
        else:
            if left.polar == 0:
                # 有极性 - 无极性 = 有极性者的极性
                yield Node('-', right, left, right.polar)
                # （无极性 - 有极性 = 舍弃）
                # （有极性 - 有极性 = 舍弃）
            if right.polar == 0:
                yield Node('-', left, right, left.polar)            # 同上
    # 乘法：两个孩子都不能是除号；左孩子还不能是乘号；
    #       若右孩子是乘号，则左孩子和右孩子的左孩子要满足单调性
    if left.ch not in '*/' and right.ch != '/' and (right.ch != '*' or left.id < right.left.id):
        if left.polar == 0 or right.polar == 0:
            # 无极性 * 无极性 = 无极性
            yield Node('*', left, right, left.polar + right.polar)
            # 有极性 * 无极性 = 有极性者的极性
        elif left.polar > 0:
            # 正极性 * 有极性 = 右子树极性
            yield Node('*', left, right, right.polar)
            # （负极性 * 有极性 = 舍弃）
    # 除法：两个孩子都不能是除号
    if left.ch != '/' and right.ch != '/':
        if left.polar == 0 or right.polar == 0:
            # 无极性 / 无极性 = 无极性
            yield Node('/', left, right, left.polar + right.polar)
            # 有极性 / 无极性 = 有极性者的极性
            # 无极性 / 有极性 = 有极性者的极性
            yield Node('/', right, left, left.polar + right.polar)  # 同上
        else:
            if left.polar > 0:
                # 正极性 / 有极性 = 右子树极性
                yield Node('/', left, right, right.polar)
                # （负极性 / 有极性 = 舍弃）
            if right.polar > 0:
                yield Node('/', right, left, left.polar)            # 同上

# 枚举由 n 个变量组成的算式


def enum(n, actions):
    def DFS(trees, minj):                                           # trees 为当前算式列表，minj 为第二棵子树的最小下标
        if len(trees) == 1:
            yield str(trees[0])                                     # 只剩一个算式，输出
            return
        for j in range(minj, len(trees)):                           # 枚举第二棵子树
            for i in range(j):                                      # 枚举第一棵子树
                for node in actions(trees[i], trees[j]):            # 枚举运算符
                    node.id = trees[-1].id + 1                      # 为新结点赋予 id
                    new_trees = [treesk for k, treesk in enumerate(
                        trees) if k != i and k != j] + [node]
                    # 从列表中去掉两棵子树，并加入运算结果
                    new_minj = j - 1 if actions in (smart2, smart4) else 1
                    # 若 actions 函数去重，则此处也避免「独立运算顺序不唯一」造成的重复
                    for expression in DFS(new_trees, new_minj):     # 递归下去
                        yield expression

    trees = [Node(chr(ord('a') + i), id=i)
             for i in range(n)]           # 初始时有 n 个由单变量组成的算式
    return DFS(trees, 1)


def main(n):
    '''
    >>> main(4)
    Expressions with only + and *:
      Smart: 52 expressions, 52 distinct expressions, 52 distinct values
      Naive: 1152 expressions, 528 distinct expressions, 52 distinct values
    Expressions with +, -, * and /:
      Smart: 1170 expressions, 1170 distinct expressions, 1170 distinct values
      Naive: 9216 expressions, 5856 distinct expressions, 1170 distinct values
    '''
    # 给变量赋予一些随机数，用于统计不等价算式有多少个
    global a, b, c, d, e, f, g, h
    a = Fraction(3141592)
    b = Fraction(6535897)
    c = Fraction(9323846)
    d = Fraction(2643383)
    e = Fraction(2795028)
    f = Fraction(8419716)
    g = Fraction(9399375)
    h = Fraction(1058209)

    # 仅考虑加法与乘法
    print('Expressions with only + and *:')

    # 用去重的 actions 函数枚举算式
    smart_exps = list(enum(n, smart2))
    smart_uniq_exps = set(smart_exps)
    smart_uniq_values = set(eval(ex) for ex in smart_uniq_exps)
    print(f'  Smart: {len(smart_exps)} expressions, {len(smart_uniq_exps)} distinct expressions, {len(smart_uniq_values)} distinct values')

    # 用不去重的 actions 函数枚举算式
    naive_exps = list(enum(n, naive2))
    naive_uniq_exps = set(naive_exps)
    naive_uniq_values = set(eval(ex) for ex in naive_uniq_exps)
    print(f'  Naive: {len(naive_exps)} expressions, {len(naive_uniq_exps)} distinct expressions, {len(naive_uniq_values)} distinct values')

    # 考虑加减乘除
    print('Expressions with +, -, * and /:')

    # 用去重的 actions 函数枚举算式
    smart_exps = list(enum(n, smart4))
    smart_uniq_exps = set(smart_exps)
    smart_uniq_values = set(eval(ex) for ex in smart_uniq_exps)
    print(f'  Smart: {len(smart_exps)} expressions, {len(smart_uniq_exps)} distinct expressions, {len(smart_uniq_values)} distinct values')

    # 用不去重的 actions 函数枚举算式
    naive_exps = list(enum(n, naive4))
    naive_uniq_exps = set(naive_exps)
    naive_uniq_values = set(eval(ex) for ex in naive_uniq_exps)
    print(f'  Naive: {len(naive_exps)} expressions, {len(naive_uniq_exps)} distinct expressions, {len(naive_uniq_values)} distinct values')


if __name__ == "__main__":
    from doctest import testmod
    testmod()
    # 输入变量个数
    n = int(input("Input the number of variables: "))
    # 主程序由此开始
    main(n)
