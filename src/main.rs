mod cpp;

extern crate pest;
#[macro_use]
extern crate pest_derive;

use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::Pairs};

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

fn parse_expr(pairs: Pairs<Rule>, pratt: &PrattParser<Rule>) -> i32 {
    pratt
        .map_primary(|primary| match primary.as_rule() {
            Rule::int  => primary.as_str().parse().unwrap(),
            Rule::expr => parse_expr(primary.into_inner(), pratt), // from "(" ~ expr ~ ")"
            _          => unreachable!(),
        })
        .map_prefix(|op, rhs| match op.as_rule() {
            Rule::neg  => -rhs,
            Rule::mmp  => rhs - 1,
            Rule::ppp  => rhs + 1,
            _          => unreachable!(),
        })
        .map_postfix(|lhs, op| match op.as_rule() {
            Rule::mm   => lhs - 1,
            Rule::pp   => lhs + 1,
            _          => unreachable!(),
        })
        .map_infix(|lhs, op, rhs| match op.as_rule() {
            Rule::add  => lhs + rhs,
            Rule::sub  => lhs - rhs,
            Rule::and  => lhs & rhs,
            Rule::or   => lhs | rhs,
            Rule::xor  => lhs ^ rhs,
            _          => unreachable!(),
        })
        .parse(pairs)
}

fn main() {
    assert_eq!(cpp::process_str("
     #if FOO
     foo text /* Bobby */
     #endif
     bar text", cpp::Context::new().define("FOO", "1")).unwrap(), "
     foo text 
     bar text");
    println!("{}", cpp::process_str("#define ZOBI
     #define FOO FOO_BAR // JR
     #ifdef FOO
     foo text
     #endif
     FOO /*bar text
     Dallas
     */ Ewing", &mut cpp::Context::new()).unwrap());
    let mut output = Vec::new();
    let result = cpp::process("#define ZOBI
    #define FOO FOO_BAR 
     #ifdef FOO
     foo text
     #endif
     FOO bar text".as_bytes(), &mut output, &mut cpp::Context::new());
    println!("Result: {:?}", result);
    let pratt =
        PrattParser::new()
        .op(Op::infix(Rule::add, Assoc::Left) | Op::infix(Rule::sub, Assoc::Left))
        .op(Op::infix(Rule::and, Assoc::Left) | Op::infix(Rule::or, Assoc::Left) | Op::infix(Rule::xor, Assoc::Left))
        .op(Op::postfix(Rule::mm) | Op::postfix(Rule::pp))
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::mmp) | Op::prefix(Rule::ppp));
    let pairs = Cc2600Parser::parse(Rule::program, "\n1+2&3+2|3").unwrap_or_else(|e| panic!("{}", e)).next().unwrap();
    for pair in pairs.into_inner() {
        match pair.as_rule() {
            Rule::expr => {
                let result = parse_expr(pair.into_inner(), &pratt);
                println!("Result : {}", result);
            },
            Rule::EOI => break,
            _ => unreachable!()
        }
    }
}
