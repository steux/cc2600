use std::io::BufReader;
use std::fs::File;
use std::collections::HashMap;

use log::debug;

use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::{Pair, Pairs}, error::LineColLocation};

use crate::error::Error;
use crate::cpp;
use crate::Args;
use crate::generate::generate_asm;

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

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
    pub zeropage: bool,
    pub size: usize,
}

#[derive(Debug)]
pub enum Operation {
    Add,
    Sub,
    Assign,
}

#[derive(Debug)]
pub enum Subscript {
    None,
    X,
    Y
}

#[derive(Debug)]
pub enum Expr<'a>{
    Integer(i32),
    Var((&'a str, Subscript)),
    BinOp {
        lhs: Box<Expr<'a>>,
        op: Operation,
        rhs: Box<Expr<'a>>,
    },
    Neg(Box<Expr<'a>>),
}

#[derive(Debug)]
pub enum Statement<'a> {
    Block(Vec<StatementLoc<'a>>),
    Expression(Expr<'a>)
}

#[derive(Debug)]
pub struct StatementLoc<'a> {
    pub pos: usize,
    pub statement: Statement<'a>
}

#[derive(Debug)]
pub struct Function<'a> {
    order: usize,
    pub code: StatementLoc<'a>,
}

pub struct State<'a> {
    variables: HashMap<String, Variable>,
    functions: HashMap<String, Function<'a>>,
    pratt: PrattParser<Rule>,
    mapped_lines: &'a Vec::<(std::rc::Rc::<String>,u32,Option<(std::rc::Rc::<String>,u32)>)>,
    pub preprocessed_utf8: &'a str,
}

impl<'a> State<'a> {
    pub fn sorted_variables(&self) -> Vec<(&String, &Variable)> {
        let mut v: Vec<(&String, &Variable)> = self.variables.iter().collect();
        v.sort_by(|a, b| a.1.order.cmp(&b.1.order));
        v
    }
    pub fn get_variable(&self, name: &str) -> &Variable {
        self.variables.get(name).unwrap()
    }
    pub fn sorted_functions(&self) -> Vec<(&String, &Function)> {
        let mut v: Vec<(&String, &Function)> = self.functions.iter().collect();
        v.sort_by(|a, b| a.1.order.cmp(&b.1.order));
        v
    }
}

pub fn syntax_error<'a>(state: &State<'a>, message: &str, loc: usize) -> Error
{
    let mut line_number: usize = 0;
    let mut char_number = 0;
    for c in state.preprocessed_utf8.chars() {
        if c == '\n' { line_number += 1; }
        char_number += 1;
        if char_number == loc { break; }
    }
    let included_in = match &state.mapped_lines[line_number].2 {
        None => None,
        Some(iin) => Some((iin.0.to_string(), iin.1))
    };
    Error::Syntax {
        filename: state.mapped_lines[line_number].0.to_string(),
        line: state.mapped_lines[line_number].1,
        included_in,
        msg: message.to_string()
    }
}

fn parse_int(p: Pair<Rule>) -> i32
{
    match p.as_rule() {
        Rule::decimal => p.as_str().parse::<i32>().unwrap(),
        Rule::hexadecimal => i32::from_str_radix(&p.as_str()[2..], 16).unwrap(),
        Rule::octal => i32::from_str_radix(p.as_str(), 8).unwrap(),
        _ => {
            unreachable!()
        }
    }
}

fn parse_var<'a>(state: &State<'a>, pairs: Pairs<'a, Rule>) -> Result<(&'a str, Subscript), Error>
{
    let mut p = pairs.into_iter();
    let px = p.next().unwrap();
    let varname = px.as_str();
    let subscript = match p.next() {
        Some(pair) => {
            match pair.as_str() {
                "[X]" => Subscript::X,
                "[Y]" => Subscript::Y,
                _ => return Err(syntax_error(state, "Bad array subscript (only X and Y are supported)", pair.as_span().start()))  
            }
        },
        None => Subscript::None
    };
    match state.variables.get(varname) {
        Some(_var) => {
            // TODO: Check subscript is correct
            Ok((varname, subscript))
        },
        None => Err(syntax_error(state, "Unknown identifier", px.as_span().start()))
    }
}

fn parse_expr<'a>(state: &State<'a>, pairs: Pairs<'a, Rule>) -> Result<Expr<'a>, Error>
{
    state.pratt
        .map_primary(|primary| -> Result<Expr<'a>, Error> {
            match primary.as_rule() {
                Rule::int => Ok(Expr::Integer(parse_int(primary.into_inner().next().unwrap()))),
                Rule::expr => Ok(parse_expr(state, primary.into_inner())?),
                Rule::var => Ok(Expr::Var(parse_var(state, primary.into_inner())?)),
                rule => unreachable!("Expr::parse expected atom, found {:?}", rule),
            }
        })
        .map_infix(|lhs, op, rhs| {
            let op = match op.as_rule() {
                Rule::add => Operation::Add,
                Rule::sub => Operation::Sub,
                Rule::assign => Operation::Assign,
                rule => unreachable!("Expr::parse expected infix operation, found {:?}", rule),
            };
            Ok(Expr::BinOp {
                lhs: Box::new(lhs?),
                op,
                rhs: Box::new(rhs?),
            })
        })
        .map_prefix(|op, rhs| match op.as_rule() {
            Rule::neg => Ok(Expr::Neg(Box::new(rhs?))),
            _ => unreachable!(),
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
                        zeropage: true,
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

fn compile_statement<'a>(state: &State<'a>, pair: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
{
    let pos = pair.as_span().start();
    match pair.as_rule() {
        Rule::expr => {
            let expr = parse_expr(state, pair.into_inner())?;
            Ok(StatementLoc {
                pos, statement: Statement::Expression(expr)
            })
        },
        _ => {
            debug!("What's this ? {:?}", pair);
            unreachable!()
        }
    }
}

fn compile_block<'a>(state: &State<'a>, p: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
{
    let pos = p.as_span().start();
    let mut statements = Vec::<StatementLoc>::new();
    for pair in p.into_inner() {
        match pair.as_rule() {
            Rule::statement => {
                statements.push(compile_statement(state, pair.into_inner().next().unwrap())?)
            },
            _ => {
                debug!("What's this ? {:?}", pair);
                unreachable!()
            }
        }
    }
    Ok(StatementLoc {
        pos, statement: Statement::Block(statements) 
    })
}

fn compile_func_decl<'a>(state: &mut State<'a>, pairs: Pairs<'a, Rule>) -> Result<(), Error>
{
    let mut p = pairs.into_iter();
    let pair = p.next().unwrap();
    match pair.as_rule() {
        Rule::var_name => {
            let name = pair.as_str();
            let pair = p.next().unwrap();
            match pair.as_rule() {
                Rule::block => {
                    let code = compile_block(state, pair)?;
                    state.functions.insert(name.to_string(), Function {
                        order: state.functions.len(),
                        code 
                    });
                },
                _ => {
                    debug!("What's this ? {:?}", pair);
                    unreachable!()
                }
            }
        },
        _ => {
            debug!("What's this ? {:?}", pair);
            unreachable!()
        }
    }
    Ok(())
}

fn compile_decl<'a>(state: &mut State<'a>, pairs: Pairs<'a, Rule>) -> Result<(), Error> 
{
    for pair in pairs {
        match pair.as_rule() {
            Rule::var_decl => {
                compile_var_decl(state, pair.into_inner())?;
            },
            Rule::func_decl => {
                compile_func_decl(state, pair.into_inner())?;
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
    
    let pratt =
        PrattParser::new()
        .op(Op::infix(Rule::comma, Assoc::Left))
        .op(Op::infix(Rule::assign, Assoc::Right))
        .op(Op::infix(Rule::add, Assoc::Left) | Op::infix(Rule::sub, Assoc::Left))
        .op(Op::infix(Rule::and, Assoc::Left) | Op::infix(Rule::or, Assoc::Left) | Op::infix(Rule::xor, Assoc::Left))
        .op(Op::postfix(Rule::mm) | Op::postfix(Rule::pp))
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::mmp) | Op::prefix(Rule::ppp));
    
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

    let preprocessed_utf8 = std::str::from_utf8(&preprocessed)?;
    
    // Prepare the state
    let mut state = State {
        variables: HashMap::new(),
        functions: HashMap::new(),
        pratt,
        mapped_lines: &mapped_lines,
        preprocessed_utf8
    };

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

    // Generate assembly code from compilation output (abstract syntax tree)
    generate_asm(&mut state, &args.output)?;
    Ok(())
}

