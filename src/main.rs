mod cpp;

use std::io::BufReader;
use std::fs::File;

extern crate pest;
#[macro_use]
extern crate pest_derive;
use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::Pairs};

use clap::Parser as ClapParser;

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

/// C compiler for Atari 2600
#[derive(ClapParser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Input file name
    input: String,

    /// Output file name
    #[arg(short, long, default_value="out.a")]
    output: String
}

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

fn main() -> Result<(), cpp::Error> {
    env_logger::init();

    let args = Args::parse();
    
    let mut preprocessed = Vec::new();
    let f = File::open(args.input)?;
    let f = BufReader::new(f);

    let mut context = cpp::Context::new();

    let mapped_lines = cpp::process(f, &mut preprocessed, &mut context)?;
    println!("Mapped lines: {mapped_lines:?}");

    let pratt =
        PrattParser::new()
        .op(Op::infix(Rule::add, Assoc::Left) | Op::infix(Rule::sub, Assoc::Left))
        .op(Op::infix(Rule::and, Assoc::Left) | Op::infix(Rule::or, Assoc::Left) | Op::infix(Rule::xor, Assoc::Left))
        .op(Op::postfix(Rule::mm) | Op::postfix(Rule::pp))
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::mmp) | Op::prefix(Rule::ppp));
    let preprocessed_utf8 = std::str::from_utf8(&preprocessed)?;
    let pairs = Cc2600Parser::parse(Rule::program, &preprocessed_utf8).unwrap_or_else(|e| panic!("{}", e)).next().unwrap();
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
    Ok(())
}
