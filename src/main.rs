mod cpp;
mod error;

use error::Error;

use std::io::prelude::*;
use std::io::BufReader;
use std::fs::File;

use log::debug;

extern crate pest;
#[macro_use]
extern crate pest_derive;
use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::Pairs, error::LineColLocation};

use clap::Parser as ClapParser;

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

/// Subset of C compiler for the Atari 2600
#[derive(ClapParser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// Input file name
    input: String,

    /// Preprocessor definitions
    #[arg(short='D')]
    defines: Vec<String>,

    /// Include directories
    #[arg(short='I')]
    include_directories: Vec<String>,

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
struct Variable {
    name: String,
    var_type: VariableType,
    size: usize,
}

#[derive(Default, Debug)]
struct Function {
    
}

#[derive(Default, Debug)]
struct State {
    variables: Vec<Variable>,
    functions: Vec<Function>
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
                    state.variables.push(Variable {name, var_type, size});
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

fn generate_asm(state: &State, filename: &str) -> Result<(), Error> 
{
    let mut file = File::create(filename)?;
    file.write_all(b"\tPROCESSOR 6502\n")?;
    file.write_all(b"\tINCLUDE \"vcs.h\"\n\n")?;
    file.write_all(b"\tSEG.U variables\n\tORG $80\n\n")?;
    for v in &state.variables {
        file.write_all(format!("{:23}\tds {}\n", v.name, v.size).as_bytes())?; 
    }
    Ok(())
}

fn compile() -> Result<(), Error> {
    env_logger::init();

    let args = Args::parse();
    
    let mut preprocessed = Vec::new();
    let f = File::open(&args.input)?;
    let f = BufReader::new(f);

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
    
    let mut state = State::default();
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

fn main() {
    match compile() {
        Err(e) => {
            eprintln!("{}", e);
            std::process::exit(1) 
        },
        Ok(_) => std::process::exit(0) 
    }
}
