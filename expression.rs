#![feature(box_syntax)]

use std::collections::VecDeque;


#[derive(Clone,Debug)]
struct Node {
    ch: char,
    l: Box<Option<Node>>,
    r: Box<Option<Node>>,
    polar: i32,
    id: i32,
}

impl Node {
    fn new(ch: char, l: &Node, r: &Node, polar: i32) -> Self {
        Self { ch, l: box Some(l.clone()), r: box Some(r.clone()), polar, id: 0 }
    }
    fn symbol(id: i32) -> Self {
        Self {
            ch: ('a' as u8 + id as u8) as char,
            l: Box::new(Default::default()),
            r: Box::new(Default::default()),
            polar: 0,
            id,
        }
    }
}

fn actions(l: &Node, r: &Node) -> Vec<Node> {
    let mut res = vec![];

    // 加法：两个孩子都不能是减号；左孩子还不能是加号；
    //       若右孩子是加号，则左孩子和右孩子的左孩子要满足单调性
    if l.ch != '+' && l.ch != '-' && r.ch != '-' && (r.ch != '+' || l.id < r.l.clone().unwrap().id) {
        if l.polar == 0 || r.polar == 0 {
            res.push(Node::new('+', l, r, l.polar + r.polar)); // 无极性 + 无极性 = 无极性
            // 有极性 + 无极性 = 有极性者的极性
        } else {
            res.push(Node::new('+', &l, &r, r.polar)); // 有极性 + 有极性 = 右子树极性
        }
    }
    // 减法：两个孩子都不能是减号
    if l.ch != '-' && r.ch != '-' {
        if l.polar == 0 && r.polar == 0 {
            // 无极性 - 无极性：
            res.push(Node::new('-', &l, &r, 1)); // 正过来减是正极性
            res.push(Node::new('-', &r, &l, -1)); // 反过来减是负极性
        } else {
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
    if l.ch != '*' && l.ch != '/' && r.ch != '/' && (r.ch != '*' || l.id < r.l.clone().unwrap().id) {
        if l.polar == 0 || r.polar == 0 {
            res.push(Node::new('*', &l, &r, l.polar + r.polar)); // 无极性 * 无极性 = 无极性
            // 有极性 * 无极性 = 有极性者的极性
        } else if l.polar > 0 {
            res.push(Node::new('*', &l, &r, r.polar)); // 正极性 * 有极性 = 右子树极性
            // （负极性 * 有极性 = 舍弃）
        }
    }
    // 除法：两个孩子都不能是除号
    if l.ch != '/' && r.ch != '/' {
        if l.polar == 0 || r.polar == 0 {
            res.push(Node::new('/', &l, &r, l.polar + r.polar)); // 无极性 / 无极性 = 无极性
            // 有极性 / 无极性 = 有极性者的极性
            // 无极性 / 有极性 = 有极性者的极性
            res.push(Node::new('/', &r, &l, l.polar + r.polar)); // 同上
        } else {
            if l.polar > 0 {
                res.push(Node::new('/', &l, &r, r.polar)); // 正极性 / 有极性 = 右子树极性
            }

            // （负极性 / 有极性 = 舍弃）
            if r.polar > 0 {
                res.push(Node::new('/', &r, &l, l.polar)); // 同上
            }
        }
    }
    return res;
}

// 枚举由 n 个变量组成的算式
// trees 为当前算式列表，mi 为第二棵子树的最小下标
fn dfs(trees: &mut VecDeque<Node>, mi: i32, total: &mut u64, neutral: &mut u64, pos: &mut u64, neg: &mut u64) {

    // 只剩一个算式
    if trees.len() == 1 {
        // 计入所有算式总数
        *total += 1;
        // 计入相应极性算式的总数
        match trees[0].polar {
            1 => { *pos += 1 }
            0 => { *neutral += 1 }
            -1 => { *neg += 1 }
            _ => unreachable!()
        }
        // 告诉用户：我还活着！
        if *total % 1000000 == 0 {
            println!("[Partial] Found {} expressions: {} : {} : {}", total, neutral, pos, neg);
        }
        return;
    }

    let new_id = trees.back().unwrap().id + 1;
    let mut j = mi as usize;
    // 枚举第二棵子树
    while j < trees.len() {
        let right = trees.pop_front().unwrap();
        let mut i = 0;
        while i < j {
            let left = trees.pop_front().unwrap();
            for mut result in actions(&left, &right) {
                result.id = new_id;
                trees.push_back(result);
                dfs(trees, (j - 1) as i32, total,  neutral,  pos, neg);
                trees.pop_back();
            }
            trees.push_back(left);
            i += 1
        }
        trees.push_back(right);
        j += 1
    }
}

#[test]
fn main() {
    let (mut total, mut neutral, mut pos, mut neg) = (0u64, 0u64, 0u64, 0u64);
    let mut trees = VecDeque::new();
    for i in 0..4 {
        trees.push_back(Node::symbol(i))
    };
    dfs(&mut trees, 1, &mut total,  &mut neutral,  &mut pos,  &mut neg);
    println!("[Partial] Found {} expressions: {} : {} : {}", total, neutral, pos, neg);
}
