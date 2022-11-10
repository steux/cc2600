use std::io::BufReader;
use std::fs::File;
use std::collections::HashMap;

use log::debug;

use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::Pairs, error::LineColLocation};

use crate::error::Error;
use crate::cpp;
use crate::Args;
use crate::generate::generate_asm;

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

/*
                    Rule::expr => {
                        let result = parse_expr(pair.into_inner(), &pratt);
                        println!("Result : {}", result);
                    },
                    Rule::EOI => break,
                    */

#[derive(Default, Debug, Copy, Clone)]
enum VariableType {
    UnsignedChar,
    #[default]
    SignedChar,
}

#[derive(Default, Debug)]
pub struct Variable {
    order: usize,
    var_type: VariableType,
    pub size: usize,
}

#[derive(Default, Debug)]
struct Function {
    
}

#[derive(Debug)]
pub struct State {
    variables: HashMap<String, Variable>,
    functions: Vec<Function>
}

impl State {
    pub fn sorted_variables(&self) -> Vec<(&String, &Variable)> {
        let mut v: Vec<(&String, &Variable)> = self.variables.iter().collect();
        v.sort_by(|a, b| a.1.order.cmp(&b.1.order));
        v
    }
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

fn compile_var_decl(state: &mut State, pairs: Pairs<Rule>) -> Result<(), Error>
{
    let mut var_type = VariableType::SignedChar;
    for pair in pairs {
        match pair.as_rule() {
            Rule::var_type => {
                let sign = pair.into_inner().next().unwrap();
                if sign.as_str().eq("unsigned") {
                    var_type = VariableType::UnsignedChar;
                }
            },
            Rule::var_name_ex => {
                let mut p = pair.into_inner();
                let name = p.next().unwrap().as_str().to_string();
                let size = match p.next() {
                    Some(x) => x.as_str().parse::<usize>().unwrap(),
                    None => 1 
                };
                if name != "X" && name != "Y" {
                    state.variables.insert(name, Variable {
                        order: state.variables.len(),
                        var_type, size});
                }
            },
            _ => {
                debug!("What's this ? {:?}", pair);
                unreachable!()
            }
        }
    }
    Ok(())
}

fn compile_decl(mut state: &mut State, pairs: Pairs<Rule>) -> Result<(), Error> 
{
    for pair in pairs {
        match pair.as_rule() {
            Rule::var_decl => {
                compile_var_decl(&mut state, pair.into_inner())?;
            },
            Rule::func_decl => {
                debug!("func decl: {:?}", pair);
            },
            _ => {
                debug!("What's this ? {:?}", pair);
                unreachable!()
            }
        }
    }
    Ok(())
}

pub fn compile(args: &Args) -> Result<(), Error> {
    let mut preprocessed = Vec::new();
    let f = File::open(&args.input)?;
    let f = BufReader::new(f);
    
    // Prepare the state
    let mut state = State {
        variables: HashMap::new(),
        functions: Vec::new()
    };

    // Prepare the context
    let mut context = cpp::Context::new(&args.input);
    context.include_directories = args.include_directories.clone();
    context.define("__ATARI2600__", "1");
    for i in &args.defines {
        let mut s = i.splitn(2, "=");
        let def = s.next().unwrap();
        let value = s.next().unwrap_or("1");
        context.define(def, value);
    }

    // Start preprocessor
    let mapped_lines = cpp::process(f, &mut preprocessed, &mut context)?;
    debug!("Mapped lines = {:?}", mapped_lines);

    let pratt =
        PrattParser::new()
        .op(Op::infix(Rule::add, Assoc::Left) | Op::infix(Rule::sub, Assoc::Left))
        .op(Op::infix(Rule::and, Assoc::Left) | Op::infix(Rule::or, Assoc::Left) | Op::infix(Rule::xor, Assoc::Left))
        .op(Op::postfix(Rule::mm) | Op::postfix(Rule::pp))
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::mmp) | Op::prefix(Rule::ppp));
    
    let preprocessed_utf8 = std::str::from_utf8(&preprocessed)?;
    let r = Cc2600Parser::parse(Rule::program, &preprocessed_utf8);
    match r {
        Err(e) => {
            let mut ex = e.clone();
            let filename;
            let line;
            ex.line_col = match e.line_col {
                LineColLocation::Pos((l, c)) => {
                    filename = mapped_lines[l - 1].0.clone();
                    line = mapped_lines[l - 1].1;
                    LineColLocation::Pos((mapped_lines[l - 1].1 as usize, c))
                },
                LineColLocation::Span((l1, c1), (l2, c2)) => {
                    filename = mapped_lines[l1 - 1].0.clone();
                    line = mapped_lines[l1 - 1].1;
                    LineColLocation::Span((mapped_lines[l1 - 1].1 as usize, c1), (mapped_lines[l2 - 1].1 as usize, c2))
                },
            };
            eprintln!("{}", ex);
            return Err(Error::Syntax {
                filename: filename.to_string(), included_in: None, line,
                msg: e.variant.message().to_string() })
        }
        Ok(mut p) => {
            let pairs = p.next().unwrap();
            for pair in pairs.into_inner() {
                match pair.as_rule() {
                    Rule::decl => {
                        compile_decl(&mut state, pair.into_inner())?;
                    },
                    Rule::EOI => break,
                    _ => {
                        debug!("What's this ? {:?}", pair);
                        unreachable!()
                    }
                }
            }
        }
    };
    generate_asm(&mut state, &args.output)?;
    Ok(())
}

