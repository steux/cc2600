use std::collections::HashMap;

use log::debug;

use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::{Pair, Pairs}, error::LineColLocation};
use std::io::{BufRead, Write};

use crate::error::Error;
use crate::cpp;
use crate::Args;
use crate::generate::generate_asm;

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum VariableType {
    Char,
    Short,
    CharPtr,
}

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum VariableMemory {
    ROM(u32),
    Zeropage,
    Superchip,
}

#[derive(Debug, PartialEq)]
pub enum VariableDefinition {
    None,
    Value(i32),
    Array(Vec<i32>),
}

#[derive(Debug)]
pub struct Variable {
    order: usize,
    pub var_type: VariableType,
    pub var_const: bool,
    pub signed: bool,
    pub memory: VariableMemory,
    pub size: usize,
    pub alignment: usize,
    pub def: VariableDefinition 
}

#[derive(Debug, PartialEq, Copy, Clone)]
pub enum Operation {
    Mul(bool),
    Div(bool),
    Add(bool),
    Sub(bool),
    And(bool),
    Or(bool),
    Xor(bool),
    Brs(bool),
    Bls(bool),
    Assign,
    Eq,
    Neq,
    Gt,
    Gte,
    Lt,
    Lte,
    Land,
    Lor,
    TernaryCond1,
    TernaryCond2
}

#[derive(Debug, Clone)]
pub enum Expr<'a> {
    Nothing,
    Integer(i32),
    Identifier((&'a str, Box<Expr<'a>>)),
    FunctionCall(Box<Expr<'a>>),
    BinOp {
        lhs: Box<Expr<'a>>,
        op: Operation,
        rhs: Box<Expr<'a>>,
    },
    Neg(Box<Expr<'a>>),
    Not(Box<Expr<'a>>),
    BNot(Box<Expr<'a>>),
    MinusMinus(Box<Expr<'a>>, bool),
    PlusPlus(Box<Expr<'a>>, bool),
    Deref(Box<Expr<'a>>),
}

#[derive(Debug, Clone)]
pub enum Statement<'a> {
    Block(Vec<StatementLoc<'a>>),
    Expression(Expr<'a>),
    For { 
        init: Expr<'a>,
        condition: Expr<'a>,
        update: Expr<'a>,
        body: Box<StatementLoc<'a>>
    },
    If {
        condition: Expr<'a>,
        body: Box<StatementLoc<'a>>,
        else_body: Option<Box<StatementLoc<'a>>>
    },
    While {
        condition: Expr<'a>,
        body: Box<StatementLoc<'a>>,
    },
    DoWhile {
        body: Box<StatementLoc<'a>>,
        condition: Expr<'a>,
    },
    Switch {
        expr: Expr<'a>,
        cases: Vec<(Vec<i32>, Vec<StatementLoc<'a>>)>
    },
    Break,
    Continue,
    Return,
    Asm(&'a str),
    Strobe(Expr<'a>),
    Load(Expr<'a>),
    Store(Expr<'a>),
    CSleep(i32),
    Goto(&'a str),
}

#[derive(Debug, Clone)]
pub struct StatementLoc<'a> {
    pub pos: usize,
    pub label: Option<String>,
    pub statement: Statement<'a>
}

#[derive(Debug)]
pub struct Function<'a> {
    order: usize,
    pub inline: bool,
    pub bank: u32,
    pub code: StatementLoc<'a>,
}

pub struct CompilerState<'a> {
    variables: HashMap<String, Variable>,
    pub functions: HashMap<String, Function<'a>>,
    pratt: PrattParser<Rule>,
    mapped_lines: &'a Vec::<(std::rc::Rc::<String>,u32,Option<(std::rc::Rc::<String>,u32)>)>,
    pub preprocessed_utf8: &'a str,
}

impl<'a> CompilerState<'a> {
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

pub fn syntax_error<'a>(state: &CompilerState<'a>, message: &str, loc: usize) -> Error
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

fn parse_identifier<'a>(state: &CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<(&'a str, Box<Expr<'a>>), Error>
{
    let mut p = pairs.into_iter();
    let px = p.next().unwrap();
    let varname = px.as_str();
    let subscript = match p.next() {
        Some(pair) => {
            let expr = parse_expr(state, pair.into_inner())?;
            Box::new(expr)
        },
        None => Box::new(Expr::Nothing), 
    };
    if varname.eq("X") || varname.eq("Y") {
        match *subscript {
            Expr::Nothing => return Ok((varname, subscript)),
            _ => return Err(syntax_error(state, &format!("No subscript for {} index", varname), px.as_span().start())),

        }
    }
    match state.variables.get(varname) {
        Some(_var) => {
            // TODO: Check subscript is correct
            Ok((varname, subscript))
        },
        None => {
            match state.functions.get(varname) {
                Some(_var) => {
                    Ok((varname, subscript))
                },
                None => Err(syntax_error(state, &format!("Unknown identifier {}", varname), px.as_span().start()))
            }
        }
    }
}

fn parse_expr<'a>(state: &CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<Expr<'a>, Error>
{
    state.pratt
        .map_primary(|primary| -> Result<Expr<'a>, Error> {
            match primary.as_rule() {
                Rule::int => Ok(Expr::Integer(parse_int(primary.into_inner().next().unwrap()))),
                Rule::expr => Ok(parse_expr(state, primary.into_inner())?),
                Rule::identifier => Ok(Expr::Identifier(parse_identifier(state, primary.into_inner())?)),
                rule => unreachable!("Expr::parse expected atom, found {:?}", rule),
            }
        })
        .map_infix(|lhs, op, rhs| {
            let op = match op.as_rule() {
                Rule::mul => Operation::Mul(false),
                Rule::div => Operation::Div(false),
                Rule::add => Operation::Add(false),
                Rule::sub => Operation::Sub(false),
                Rule::and => Operation::And(false),
                Rule::or => Operation::Or(false),
                Rule::xor => Operation::Xor(false),
                Rule::brs => Operation::Brs(false),
                Rule::bls => Operation::Bls(false),
                Rule::eq => Operation::Eq,
                Rule::neq => Operation::Neq,
                Rule::assign => Operation::Assign,
                Rule::gt => Operation::Gt,
                Rule::gte => Operation::Gte,
                Rule::lt => Operation::Lt,
                Rule::lte => Operation::Lte,
                Rule::mulass => Operation::Mul(true),
                Rule::divass => Operation::Div(true),
                Rule::pass => Operation::Add(true),
                Rule::mass => Operation::Sub(true),
                Rule::blsass => Operation::Bls(true),
                Rule::brsass => Operation::Brs(true),
                Rule::andass => Operation::And(true),
                Rule::orass => Operation::Or(true),
                Rule::xorass => Operation::Xor(true),
                Rule::land => Operation::Land,
                Rule::lor => Operation::Lor,
                Rule::ternary_cond1 => Operation::TernaryCond1,
                Rule::ternary_cond2 => Operation::TernaryCond2,
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
            Rule::not => Ok(Expr::Not(Box::new(rhs?))),
            Rule::bnot => Ok(Expr::BNot(Box::new(rhs?))),
            Rule::deref => Ok(Expr::Deref(Box::new(rhs?))),
            Rule::mmp => Ok(Expr::MinusMinus(Box::new(rhs?), false)),
            Rule::ppp => Ok(Expr::PlusPlus(Box::new(rhs?), false)),
            _ => unreachable!(),
        })
        .map_postfix(|lhs, op| match op.as_rule() {
            Rule::mm => Ok(Expr::MinusMinus(Box::new(lhs?), true)),
            Rule::pp => Ok(Expr::PlusPlus(Box::new(lhs?), true)),
            Rule::call =>Ok(Expr::FunctionCall(Box::new(lhs?))),             
            _ => unreachable!(),
        })
        .parse(pairs)
}

// TODO: Exclude already defined variables
fn compile_var_decl(state: &mut CompilerState, pairs: Pairs<Rule>) -> Result<(), Error>
{
    let mut var_type = VariableType::Char;
    let mut var_const = false;
    let mut signed = true;
    let mut memory = VariableMemory::Zeropage;
    let mut alignment = 1;
    for pair in pairs {
        match pair.as_rule() {
            Rule::var_type => {
                for p in pair.into_inner() {
                    match p.as_rule() {
                        Rule::var_const => memory = VariableMemory::ROM(0),
                        Rule::bank => memory = VariableMemory::ROM(u32::from_str_radix(p.into_inner().next().unwrap().as_str(), 10).unwrap()),
                        Rule::superchip => memory = VariableMemory::Superchip,
                        Rule::var_sign => if p.as_str().eq("unsigned") {
                            signed = false;
                        },
                        Rule::var_simple_type => if p.as_str().eq("short") {
                            var_type = VariableType::Short;

                        },
                        Rule::aligned => alignment = p.into_inner().next().unwrap().as_str().parse::<usize>().unwrap(),
                        _ => unreachable!()
                    }
                }
            },
            Rule::id_name_ex => {
                let mut name = "";
                let mut size:usize = 1;
                let mut def = VariableDefinition::None;
                for p in pair.into_inner() {
                    match p.as_rule() {
                        Rule::pointer => var_type = VariableType::CharPtr,
                        Rule::var_const => var_const = true,
                        Rule::id_name => name = p.as_str(),
                        Rule::int => size = p.as_str().parse::<usize>().unwrap(),
                        Rule::var_def => {
                            let px = p.into_inner().next().unwrap();
                            match px.as_rule() {
                                Rule::int => def = VariableDefinition::Value(parse_int(px.into_inner().next().unwrap())),
                                Rule::array_def => {
                                    let start = px.as_span().start();
                                    if let VariableMemory::ROM(_) = memory {} else {
                                        memory = VariableMemory::ROM(0);
                                    }
                                    if var_type == VariableType::Char {
                                        var_type = VariableType::CharPtr;
                                    } else {
                                        return Err(syntax_error(state, "Array of short integers are not available", start));
                                    }
                                    let mut v = Vec::new();
                                    for pxx in px.into_inner() {
                                        v.push(parse_int(pxx.into_inner().next().unwrap()));
                                    }
                                    if size != v.len() {
                                        return Err(syntax_error(state, "Specified array size is different from actual definition", start));
                                    }
                                    def = VariableDefinition::Array(v);
                                    var_const = true;
                                },
                                _ => unreachable!()

                            } 
                        },
                        _ => unreachable!()
                    }
                }

                if name != "X" && name != "Y" {
                    state.variables.insert(name.to_string(), Variable {
                        order: state.variables.len(),
                        signed,
                        memory,
                        var_const,
                        alignment,
                        def,
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

fn compile_statement<'a>(state: &CompilerState<'a>, p: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
{
    let mut inner = p.into_inner();
    let pair = inner.next().unwrap();
    debug!("Compile statement: {:?}\ninner:{:?}", pair, inner);
    let pos = pair.as_span().start();
    match pair.as_rule() {
        Rule::label => {
            let statement = compile_statement_ex(state, inner.next().unwrap())?;
            Ok(StatementLoc { pos, label: Some(pair.into_inner().next().unwrap().as_str().to_string()), statement: statement.statement })
        },
        _ => {
            compile_statement_ex(state, pair)
        }
    }
}

fn compile_statement_ex<'a>(state: &CompilerState<'a>, pair: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
{
    let pos = pair.as_span().start();
    match pair.as_rule() {
        Rule::expr => {
            let expr = parse_expr(state, pair.into_inner())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Expression(expr)
            })
        },
        Rule::block => {
            compile_block(state, pair)
        },
        Rule::for_loop => {
            let mut p = pair.into_inner();
            let i = p.next().unwrap();
            let init = match i.as_rule() {
                Rule::nothing => Expr::Nothing,
                _ => parse_expr(state, i.into_inner())?
            };
            let condition = parse_expr(state, p.next().unwrap().into_inner())?;
            let update = parse_expr(state, p.next().unwrap().into_inner())?;
            let body = compile_statement(state, p.next().unwrap())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::For {
                    init, condition, update, body: Box::new(body) 
                }
            })
        },
        Rule::if_statement => {
            let mut p = pair.into_inner();
            let condition = parse_expr(state, p.next().unwrap().into_inner())?;
            let body = compile_statement(state, p.next().unwrap())?;
            let else_body = match p.next() {
                None => None,
                Some(px) => Some(Box::new(compile_statement(state, px)?)),
            };
            Ok(StatementLoc {
                pos, label: None, statement: Statement::If {
                    condition, body: Box::new(body), else_body 
                }
            })
        },
        Rule::do_while => {
            let mut p = pair.into_inner();
            let body = compile_statement(state, p.next().unwrap())?;
            let condition = parse_expr(state, p.next().unwrap().into_inner())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::DoWhile {
                    body: Box::new(body), condition  
                }
            })
        },
        Rule::while_do => {
            let mut p = pair.into_inner();
            let condition = parse_expr(state, p.next().unwrap().into_inner())?;
            let body = compile_statement(state, p.next().unwrap())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::While {
                    condition, body: Box::new(body) 
                }
            })
        },
        Rule::switch_statement => {
            let mut p = pair.into_inner();
            let mut cases = Vec::<(Vec<i32>, Vec<StatementLoc<'a>>)>::new();
            let expr = parse_expr(state, p.next().unwrap().into_inner())?;
            let c = p.next().unwrap().into_inner();
            debug!("Cases: {:?}", c);
            let mut case_set = (Vec::<i32>::new(), Vec::<StatementLoc<'a>>::new());
            let mut last_was_a_statement = false;
            for i in c {
                match i.as_rule() {
                    Rule::int => {
                        if last_was_a_statement {
                            cases.push(case_set.clone());
                            case_set = (Vec::<i32>::new(), Vec::<StatementLoc<'a>>::new());
                        }
                        case_set.0.push(parse_int(i.into_inner().next().unwrap()));
                    },
                    Rule::statement => {
                        case_set.1.push(compile_statement(state, i)?);
                        last_was_a_statement = true;
                    },
                    Rule::default_case => {
                        cases.push(case_set.clone());
                        let mut default_set = (Vec::<i32>::new(), Vec::<StatementLoc<'a>>::new());
                        let d = i.into_inner();
                        for j in d {
                            match j.as_rule() {
                                Rule::statement => {
                                    default_set.1.push(compile_statement(state, j)?);
                                },
                                _ => unreachable!()
                            }
                        }
                        cases.push(default_set);
                    },
                    _ => unreachable!()
                }
            }
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Switch {
                    expr, cases 
                }
            })
        },
        Rule::break_statement => {
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Break
            })
        },
        Rule::continue_statement => {
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Continue
            })
        },
        Rule::return_statement => {
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Return
            })
        },
        Rule::asm_statement => {
            let s = pair.into_inner().next().unwrap().into_inner().next().unwrap().as_str();
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Asm(s)
            })
        },
        Rule::strobe_statement => {
            let s = parse_expr(state, pair.into_inner())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Strobe(s)
            })
        },
        Rule::load_statement => {
            let s = parse_expr(state, pair.into_inner())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Load(s)
            })
        },
        Rule::store_statement => {
            let s = parse_expr(state, pair.into_inner())?;
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Store(s)
            })
        },
        Rule::csleep_statement => {
            let s = parse_int(pair.into_inner().next().unwrap().into_inner().next().unwrap());
            Ok(StatementLoc {
                pos, label: None, statement: Statement::CSleep(s)
            })
        },
        Rule::goto_statement => {
            let s = pair.into_inner().next().unwrap().as_str();
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Goto(s)
            })
        },
        Rule::nothing => {
            Ok(StatementLoc {
                pos, label: None, statement: Statement::Expression(Expr::Nothing)
            })
        },
        _ => {
            debug!("What's this ? {:?}", pair);
            unreachable!()
        }
    }
}

fn compile_block<'a>(state: &CompilerState<'a>, p: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
{
    let pos = p.as_span().start();
    let mut statements = Vec::<StatementLoc>::new();
    for pair in p.into_inner() {
        match pair.as_rule() {
            Rule::statement => {
                statements.push(compile_statement(state, pair)?)
            },
            _ => {
                debug!("What's this ? {:?}", pair);
                unreachable!()
            }
        }
    }
    Ok(StatementLoc {
        pos, label: None, statement: Statement::Block(statements) 
    })
}

// TODO: Accept undefined functions
fn compile_func_decl<'a>(state: &mut CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<(), Error>
{
    let mut inline = false;
    let mut bank = 0u32;
    let mut p = pairs.into_iter();
    let mut pair = p.next().unwrap();
    if pair.as_rule() == Rule::inline { 
        inline = true; 
        pair = p.next().unwrap();
    }
    if pair.as_rule() == Rule::bank { 
        bank = u32::from_str_radix(pair.into_inner().next().unwrap().as_str(), 10).unwrap();
        pair = p.next().unwrap();
    }
    match pair.as_rule() {
        Rule::id_name => {
            let name = pair.as_str();
            let pair = p.next().unwrap();
            match pair.as_rule() {
                Rule::block => {
                    let code = compile_block(state, pair)?;
                    state.functions.insert(name.to_string(), Function {
                        order: state.functions.len(),
                        inline,
                        bank,
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

fn compile_decl<'a>(state: &mut CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<(), Error> 
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

pub fn compile<I: BufRead, O: Write>(input: I, output: &mut O, args: &Args) -> Result<(), Error> {
    let mut preprocessed = Vec::new();
    
    let pratt =
        PrattParser::new()
        .op(Op::infix(Rule::comma, Assoc::Left))
        .op(Op::infix(Rule::assign, Assoc::Right) | Op::infix(Rule::mass, Assoc::Right) | Op::infix(Rule::pass, Assoc::Right))
        .op(Op::infix(Rule::ternary_cond1, Assoc::Right))
        .op(Op::infix(Rule::ternary_cond2, Assoc::Right))
        .op(Op::infix(Rule::lor, Assoc::Left))
        .op(Op::infix(Rule::land, Assoc::Left))
        .op(Op::infix(Rule::or, Assoc::Left))
        .op(Op::infix(Rule::xor, Assoc::Left))
        .op(Op::infix(Rule::and, Assoc::Left))
        .op(Op::infix(Rule::eq, Assoc::Left) | Op::infix(Rule::neq, Assoc::Left) | Op::infix(Rule::gt, Assoc::Left) | Op::infix(Rule::gte, Assoc::Left) | Op::infix(Rule::lt, Assoc::Left) | Op::infix(Rule::lte, Assoc::Left))
        .op(Op::infix(Rule::brs, Assoc::Left) | Op::infix(Rule::bls, Assoc::Left))
        .op(Op::infix(Rule::add, Assoc::Left) | Op::infix(Rule::sub, Assoc::Left))
        .op(Op::infix(Rule::mul, Assoc::Left) | Op::infix(Rule::div, Assoc::Left))
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::not) | Op::prefix(Rule::bnot) | Op::prefix(Rule::mmp) | Op::prefix(Rule::ppp) | Op::prefix(Rule::deref))
        .op(Op::postfix(Rule::call) | Op::postfix(Rule::mm) | Op::postfix(Rule::pp));
    
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
    let mapped_lines = cpp::process(input, &mut preprocessed, &mut context)?;
    debug!("Mapped lines = {:?}", mapped_lines);

    let preprocessed_utf8 = std::str::from_utf8(&preprocessed)?;
    
    // Prepare the state
    let mut state = CompilerState {
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
    generate_asm(&mut state, output, args.insert_code)?;
    Ok(())
}

