#![feature(box_syntax)]

use std::{
    collections::VecDeque,
    fmt::{self, Debug, Display, Formatter},
};

#[derive(Debug, Default)]
struct Counter {
    total: u64,
    zero: u64,
    pos: u64,
    neg: u64,
}

impl Counter {
    fn count(&mut self, c: &Node) {
        self.total += 1;
        match c.polar {
            1 => self.pos += 1,
            0 => self.zero += 1,
            -1 => self.neg += 1,
            _ => unreachable!(),
        }
        // 告诉用户：我还活着！
        if self.total % 1000000 == 0 {
            println!("{}", self);
        }
    }
}

impl Display for Counter {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        write!(f, "[Partial] Found {} expressions: {} : {} : {}", self.total, self.zero, self.pos, self.neg)
    }
}

#[derive(Clone)]
struct Node {
    c: char,
    l: Box<Option<Node>>,
    r: Box<Option<Node>>,
    polar: i32,
    id: i32,
}

impl Node {
    fn new(ch: char, l: &Node, r: &Node, polar: i32) -> Self {
        Self { c: ch, l: box Some(l.clone()), r: box Some(r.clone()), polar, id: 0 }
    }
    fn symbol(id: i32) -> Self {
        Self {
            c: ('a' as u8 + id as u8) as char,
            l: Box::new(Default::default()),
            r: Box::new(Default::default()),
            polar: 0,
            id,
        }
    }
}

impl Debug for Node {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        match (*self.l).clone() {
            None => write!(f, "{}", self.c),
            Some(lhs) => write!(f, "({:?} {} {:?})", lhs, self.c, (*self.r.clone()).unwrap()),
        }
    }
}

fn actions(l: &Node, r: &Node) -> Vec<Node> {
    let mut res = vec![];

    // 加法：两个孩子都不能是减号；左孩子还不能是加号；
    //       若右孩子是加号，则左孩子和右孩子的左孩子要满足单调性
    if l.c != '+' && l.c != '-' && r.c != '-' && (r.c != '+' || l.id < r.l.clone().unwrap().id) {
        if l.polar == 0 || r.polar == 0 {
            // 无极性 + 无极性 = 无极性
            // 有极性 + 无极性 = 有极性者的极性
            res.push(Node::new('+', l, r, l.polar + r.polar));
        }
        else {
            // 有极性 + 有极性 = 右子树极性
            res.push(Node::new('+', &l, &r, r.polar));
        }
    }
    // 减法：两个孩子都不能是减号
    if l.c != '-' && r.c != '-' {
        // 无极性 - 无极性：
        if l.polar == 0 && r.polar == 0 {
            // 正过来减是正极性
            res.push(Node::new('-', &l, &r, 1));
            // 反过来减是负极性
            res.push(Node::new('-', &r, &l, -1));
        }
        else {
            if l.polar == 0 {
                res.push(Node::new('-', &r, &l, r.polar))
            }
            // 有极性 - 无极性 = 有极性者的极性
            // （无极性 - 有极性 = 舍弃）
            // （有极性 - 有极性 = 舍弃）
            if r.polar == 0 {
                res.push(Node::new('-', &l, &r, l.polar));
            }
            // 同上
        }
    }
    // 乘法：两个孩子都不能是除号；左孩子还不能是乘号；
    //       若右孩子是乘号，则左孩子和右孩子的左孩子要满足单调性
    if l.c != '*' && l.c != '/' && r.c != '/' && (r.c != '*' || l.id < r.l.clone().unwrap().id) {
        if l.polar == 0 || r.polar == 0 {
            res.push(Node::new('*', &l, &r, l.polar + r.polar));
        // 无极性 * 无极性 = 无极性
        // 有极性 * 无极性 = 有极性者的极性
        }
        else if l.polar > 0 {
            res.push(Node::new('*', &l, &r, r.polar));
            // 正极性 * 有极性 = 右子树极性
            // （负极性 * 有极性 = 舍弃）
        }
    }
    // 除法：两个孩子都不能是除号
    if l.c != '/' && r.c != '/' {
        if l.polar == 0 || r.polar == 0 {
            res.push(Node::new('/', &l, &r, l.polar + r.polar));
            // 无极性 / 无极性 = 无极性
            // 有极性 / 无极性 = 有极性者的极性
            // 无极性 / 有极性 = 有极性者的极性
            res.push(Node::new('/', &r, &l, l.polar + r.polar));
        // 同上
        }
        else {
            if l.polar > 0 {
                res.push(Node::new('/', &l, &r, r.polar));
                // 正极性 / 有极性 = 右子树极性
            }
            // （负极性 / 有极性 = 舍弃）
            if r.polar > 0 {
                res.push(Node::new('/', &r, &l, l.polar));
                // 同上
            }
        }
    }
    return res;
}

// 枚举由 n 个变量组成的算式
// trees 为当前算式列表，mi 为第二棵子树的最小下标
fn dfs(trees: &mut VecDeque<Node>, mi: usize, counter: &mut Counter) {
    // 只剩一个算式
    if trees.len() == 1 {
        counter.count(&trees[0]);
        return;
    }

    let new_id = trees.back().unwrap().id + 1;
    let (mut i, mut j) = (0, mi);
    // 枚举第二棵子树
    while j < trees.len() {
        // 从列表中去掉第二棵子树
        let right = trees.swap_remove_front(j).unwrap();
        // 枚举第一棵子树
        while i < j {
            // 枚举第一棵子树
            let left = trees.swap_remove_front(i).unwrap();
            for mut result in actions(&left, &right) {
                // println!("{:#?}", result);
                result.id = new_id;
                trees.push_back(result);
                dfs(trees, j - 1, counter);
                trees.pop_back();
            }
            // 把第一棵子树加回列表
            trees.insert(i, left);
            i += 1
        }
        // 把第二棵子树加回列表
        trees.insert(j, right);
        j += 1
    }
}

#[test]
fn main() {
    const N: usize = 4;
    let mut c = Counter::default();
    let mut trees = VecDeque::new();
    for i in 0..4 {
        trees.push_back(Node::symbol(i))
    }
    dfs(&mut trees, 1, &mut c);
    println!("{}", c);
    // println!("{:#?}", trees);
}
