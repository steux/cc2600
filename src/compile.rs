/*
    cc2600 - a subset of C compiler for the Atari 2600
    Copyright (C) 2023 Bruno STEUX 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Contact info: bruno.steux@gmail.com
*/

use std::collections::HashMap;

use log::debug;

use pest::{Parser, pratt_parser::{Op, PrattParser, Assoc}, iterators::{Pair, Pairs}, error::LineColLocation};
use std::io::{BufRead, Write};

use crate::error::Error;
use crate::cpp;
use crate::Args;
use crate::build::build_cartridge;

#[derive(Parser)]
#[grammar = "cc2600.pest"]
struct Cc2600Parser;

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum VariableType {
    Char,
    Short,
    CharPtr,
    CharPtrPtr,
    ShortPtr,
}

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum VariableMemory {
    ROM(u32),
    Zeropage,
    Superchip,
    Display,
    Frequency,
    MemoryOnChip(u32),
}

#[derive(Debug, PartialEq)]
pub enum VariableDefinition {
    None,
    Value(i32),
    Array(Vec<i32>),
    ArrayOfPointers(Vec<String>),
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
    TernaryCond2,
    Comma,
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
    Sizeof(Box<Expr<'a>>),
    TmpId(String),
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
    pub code: Option<StatementLoc<'a>>,
}

pub struct CompilerState<'a> {
    pub variables: HashMap<String, Variable>,
    pub functions: HashMap<String, Function<'a>>,
    pratt: PrattParser<Rule>,
    calculator: PrattParser<Rule>,
    mapped_lines: &'a Vec::<(std::rc::Rc::<String>,u32,Option<(std::rc::Rc::<String>,u32)>)>,
    pub preprocessed_utf8: &'a str,
    pub included_assembler: Vec<String>,
    pub context: cpp::Context,
    signed_chars: bool,
    literal_counter: usize,
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
    let included_in = state.mapped_lines[line_number].2.as_ref().map(|iin| (iin.0.to_string(), iin.1));
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
    let mut p = pairs;
    let px = p.next().unwrap();
    let varname = px.as_str();
    let subscript = match p.next() {
        Some(pair) => {
            let expr = parse_expr_ex(state, pair.into_inner())?;
            Box::new(expr.0)
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
            Ok((varname, subscript))
        },
        None => {
            match state.functions.get(varname) {
                Some(_var) => {
                    match *subscript {
                        Expr::Nothing => Ok((varname, subscript)),
                        _ => Err(syntax_error(state, "No subscript for functions", px.as_span().start())),
                    }
                },
                None => Err(syntax_error(state, &format!("Unknown identifier {}", varname), px.as_span().start()))
            }
        }
    }
}

fn parse_expr<'a>(state: &mut CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<Expr<'a>, Error>
{
    let res = parse_expr_ex(state, pairs)?;
    
    // Create collected literal variables in memory
    for k in &res.1 {
        let vb = k.1.as_bytes();
        let mut v = Vec::<i32>::new();
        for c in vb.iter() {
            v.push(*c as i32);
        }
        let size = v.len();
        state.variables.insert(k.0.clone(), Variable {
            order: state.variables.len(),
            signed: false,
            memory: VariableMemory::ROM(0),
            var_const: true,
            alignment: 1,
            def: VariableDefinition::Array(v),
            var_type: VariableType::CharPtr, size});
    }
    Ok(res.0)
}

fn parse_expr_ex<'a>(state: &CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<(Expr<'a>, HashMap<String, String>), Error>
{
    let mut literal_counter = state.literal_counter;
    let mut literal_strings = HashMap::<String, String>::new(); 
    let res = state.pratt
        .map_primary(|primary| -> Result<Expr<'a>, Error> {
            match primary.as_rule() {
                Rule::int => Ok(Expr::Integer(parse_int(primary.into_inner().next().unwrap()))),
                Rule::expr => {
                    let res = parse_expr_ex(state, primary.into_inner())?;
                    for k in &res.1 {
                        literal_strings.insert(k.0.clone(), k.1.clone());
                    }
                    literal_counter += res.1.len();
                    Ok(res.0) 
                },
                Rule::identifier => Ok(Expr::Identifier(parse_identifier(state, primary.into_inner())?)),
                Rule::quoted_string => {
                    // Create a temp variable pointing to this quoted_string
                    let v = compile_quoted_string(primary.into_inner().next().unwrap().as_str());
                    let name = format!("cctmp{}", literal_counter);
                    literal_counter += 1;
                    literal_strings.insert(name.clone(), v);
                    Ok(Expr::TmpId(name))
                },
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
                Rule::comma => Operation::Comma,
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
            Rule::sizeof => Ok(Expr::Sizeof(Box::new(rhs?))),
            _ => unreachable!(),
        })
        .map_postfix(|lhs, op| match op.as_rule() {
            Rule::mm => Ok(Expr::MinusMinus(Box::new(lhs?), true)),
            Rule::pp => Ok(Expr::PlusPlus(Box::new(lhs?), true)),
            Rule::call =>Ok(Expr::FunctionCall(Box::new(lhs?))),             
            _ => unreachable!(),
        })
        .parse(pairs);
    match res {
        Ok(r) => Ok((r, literal_strings)),
        Err(x) => Err(x),
    }
}

fn parse_calc<'a>(state: &CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<i32, Error>
{
    state.calculator
        .map_primary(|primary| -> Result<i32, Error> {
            match primary.as_rule() {
                Rule::int => Ok(parse_int(primary.into_inner().next().unwrap())),
                Rule::calc_expr => Ok(parse_calc(state, primary.into_inner())?),
                rule => unreachable!("parse_calc expected atom, found {:?}", rule),
            }
        })
        .map_infix(|lhs, op, rhs| {
            let res = match op.as_rule() {
                Rule::mul => lhs.unwrap() * rhs.unwrap(),
                Rule::div => {
                    let d = rhs.unwrap();
                    if d == 0 {
                        let start = op.as_span().start();
                        return Err(syntax_error(state, "Division by zero", start));
                    }
                    lhs.unwrap() / d
                },
                Rule::add => lhs.unwrap() + rhs.unwrap(),
                Rule::sub => lhs.unwrap() - rhs.unwrap(),
                Rule::and => lhs.unwrap() & rhs.unwrap(),
                Rule::or => lhs.unwrap() | rhs.unwrap(),
                Rule::xor => lhs.unwrap() ^ rhs.unwrap(),
                Rule::brs => lhs.unwrap() >> rhs.unwrap(),
                Rule::bls => lhs.unwrap() << rhs.unwrap(),
                rule => unreachable!("parse_calc expected infix operation, found {:?}", rule),
            };
            Ok(res)
        })
        .map_prefix(|op, rhs| match op.as_rule() {
            Rule::neg => Ok(-rhs?),
            Rule::not => Ok(!rhs?),
            Rule::bnot => Ok(!rhs?),
            _ => unreachable!(),
        })
        .parse(pairs)
}

fn compile_quoted_string(s: &str) -> String 
{
    let mut i = s.chars();
    let mut v = String::new();
    while let Some(c) = i.next() {
        if c == '\\' {
            match i.next() {
                Some('0') => v.push(char::from_u32(0).unwrap()),
                Some('n') => v.push(char::from_u32(10).unwrap()),
                Some('r') => v.push(char::from_u32(13).unwrap()),
                Some('a') => v.push(char::from_u32(7).unwrap()), // Bell
                Some('b') => v.push(char::from_u32(8).unwrap()), // Backspace
                Some('t') => v.push(char::from_u32(9).unwrap()), // Tab
                Some('f') => v.push(char::from_u32(14).unwrap()), // Form feed
                Some('v') => v.push(char::from_u32(11).unwrap()), // Vertical tab
                Some(a) => v.push(a),
                _ => (),
            }
        } else {
            v.push(c);
        }
    }
    v.push(char::from_u32(0).unwrap());
    v
}

fn compile_var_decl(state: &mut CompilerState, pairs: Pairs<Rule>) -> Result<(), Error>
{
    let mut var_type_ex = VariableType::Char;
    let mut var_const = false;
    let mut signedness_specified = false;
    let mut signed = state.signed_chars;
    let mut memory = VariableMemory::Zeropage;
    let mut alignment = 1;
    for pair in pairs {
        match pair.as_rule() {
            Rule::var_type => {
                for p in pair.into_inner() {
                    //debug!("{:?}", p);
                    match p.as_rule() {
                        Rule::var_const => memory = VariableMemory::ROM(0),
                        Rule::bank => memory = VariableMemory::ROM(p.into_inner().next().unwrap().as_str().parse::<u32>().unwrap()),
                        Rule::superchip => memory = VariableMemory::Superchip,
                        Rule::display => memory = VariableMemory::Display,
                        Rule::frequency => memory = VariableMemory::Frequency,
                        Rule::var_sign => {
                            signed = p.as_str().eq("signed");
                            signedness_specified = true;
                        },
                        Rule::var_simple_type => if p.as_str().eq("short") {
                            var_type_ex = VariableType::Short;
                            if !signedness_specified { signed = true; }
                        },
                        Rule::aligned => alignment = p.into_inner().next().unwrap().as_str().parse::<usize>().unwrap(),
                        _ => unreachable!()
                    }
                }
            },
            Rule::id_name_ex => {
                let mut name = "";
                let mut size = None;
                let mut def = VariableDefinition::None;
                let mut var_type = var_type_ex;
                for p in pair.into_inner() {
                    match p.as_rule() {
                        Rule::pointer => var_type = VariableType::CharPtr,
                        Rule::var_const => var_const = true,
                        Rule::id_name => {
                            name = p.as_str();
                            let start = p.as_span().start();
                            if state.variables.get(name).is_some() {
                                return Err(syntax_error(state, &format!("Variable {} already defined", &name), start));
                            }
                        },
                        Rule::array_spec => {
                            let start = p.as_span().start();
                            if let Some(px) = p.into_inner().next() {
                                size = Some(parse_calc(state, px.into_inner())? as usize);
                            }
                            if var_type == VariableType::Char {
                                var_type = VariableType::CharPtr;
                                var_const = true;
                            } else if var_type == VariableType::CharPtr {
                                var_type = VariableType::CharPtrPtr;
                                var_const = true;
                            } else if var_type == VariableType::Short {
                                var_type = VariableType::ShortPtr;
                                var_const = true;
                            } else {
                                return Err(syntax_error(state, "Kind of array not available", start));
                            }
                        },
                        Rule::var_def => {
                            let px = p.into_inner().next().unwrap();
                            match px.as_rule() {
                                Rule::calc_expr => {
                                    let v = parse_calc(state, px.into_inner())?;
                                    def = VariableDefinition::Value(v);
                                    if var_type == VariableType::CharPtr && v > 1023 {
                                        memory = VariableMemory::MemoryOnChip(0);
                                    } 
                                },
                                Rule::array_def => {
                                    let start = px.as_span().start();
                                    memory = match memory {
                                        VariableMemory::ROM(_) | VariableMemory::Display | VariableMemory::Frequency => memory,
                                        _ => VariableMemory::ROM(0)
                                    };
                                    if var_type != VariableType::CharPtr && var_type != VariableType::CharPtrPtr && var_type != VariableType::ShortPtr {
                                        return Err(syntax_error(state, "Array definition provided for something not an array", start));
                                    }
                                    if var_type == VariableType::CharPtr || var_type == VariableType::ShortPtr {
                                        let mut v = Vec::new();
                                        for pxx in px.into_inner() {
                                            v.push(parse_calc(state, pxx.into_inner())?);
                                        }
                                        if let Some(s) = size {
                                            if s != v.len() {
                                                return Err(syntax_error(state, "Specified array size is different from actual definition", start));
                                            }
                                        }
                                        size = Some(v.len());
                                        def = VariableDefinition::Array(v);
                                    } else {
                                        let mut v = Vec::new();
                                        for pxx in px.into_inner() {
                                            match pxx.as_rule() {
                                                Rule::id_name => {
                                                    let s = pxx.as_str().to_string();
                                                    let var = state.variables.get(&s);
                                                    if let Some(vx) = var {
                                                        if vx.var_type != VariableType::CharPtr {
                                                            return Err(syntax_error(state, &format!("Reference {} should be a pointer", s), start));
                                                        }
                                                    } else {
                                                        return Err(syntax_error(state, &format!("Unknown identifier {}", s), start));
                                                    }
                                                    v.push(s);
                                                },
                                                Rule::quoted_string => {
                                                    let k = compile_quoted_string(pxx.into_inner().next().unwrap().as_str());
                                                    let name = format!("cctmp{}", state.literal_counter);
                                                    state.literal_counter += 1;
                                                    let vb = k.as_bytes();
                                                    let mut arr = Vec::<i32>::new();
                                                    for c in vb.iter() {
                                                        arr.push(*c as i32);
                                                    }
                                                    let size = v.len();
                                                    state.variables.insert(name.clone(), Variable {
                                                        order: state.variables.len(),
                                                        signed: false,
                                                        memory,
                                                        var_const: true,
                                                        alignment: 1,
                                                        def: VariableDefinition::Array(arr),
                                                        var_type: VariableType::CharPtr, size});
                                                    v.push(name);
                                                },
                                                _ => return Err(syntax_error(state, "Unexpected array value", start)),
                                            }
                                        }
                                        if let Some(s) = size {
                                            if s != v.len() {
                                                return Err(syntax_error(state, "Specified array size is different from actual definition", start));
                                            }
                                        }
                                        size = Some(v.len());
                                        def = VariableDefinition::ArrayOfPointers(v);
                                    }
                                    var_const = true;
                                },
                                Rule::quoted_string => {
                                    let start = px.as_span().start();
                                    memory = match memory {
                                        VariableMemory::ROM(_) | VariableMemory::Display | VariableMemory::Frequency => memory,
                                        _ => VariableMemory::ROM(0)
                                    };
                                    if var_type != VariableType::CharPtr {
                                        return Err(syntax_error(state, "String provided for something not a char*", start));
                                    }
                                    let s = px.into_inner().next().unwrap().as_str();
                                    let string = compile_quoted_string(s);
                                    let vb = string.as_bytes();
                                    let mut v = Vec::<i32>::new();
                                    for c in vb.iter() {
                                        v.push(*c as i32);
                                    }
                                    size = Some(v.len());
                                    def = VariableDefinition::Array(v);
                                    var_const = true;
                                },
                                _ => unreachable!()
                            } 
                        },
                        _ => unreachable!()
                    }
                }
                
                if def == VariableDefinition::None {
                    if let VariableMemory::ROM(bank) = memory {
                        memory = VariableMemory::MemoryOnChip(bank);
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
                        var_type, size: size.unwrap_or(1)});
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

fn compile_statement<'a>(state: &mut CompilerState<'a>, p: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
{
    let mut inner = p.into_inner();
    let pair = inner.next().unwrap();
    //debug!("Compile statement: {:?}\ninner:{:?}", pair, inner);
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

fn compile_statement_ex<'a>(state: &mut CompilerState<'a>, pair: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
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
            //debug!("Cases: {:?}", c);
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

fn compile_block<'a>(state: &mut CompilerState<'a>, p: Pair<'a, Rule>) -> Result<StatementLoc<'a>, Error>
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

fn compile_func_decl<'a>(state: &mut CompilerState<'a>, pairs: Pairs<'a, Rule>) -> Result<(), Error>
{
    let mut inline = false;
    let mut bank = 0u32;
    let mut p = pairs;
    let mut pair = p.next().unwrap();
    if pair.as_rule() == Rule::inline { 
        inline = true; 
        pair = p.next().unwrap();
    }
    if pair.as_rule() == Rule::bank { 
        bank = pair.into_inner().next().unwrap().as_str().parse::<u32>().unwrap();
        pair = p.next().unwrap();
    }
    if bank != 0 && inline {
        return Err(syntax_error(state, "Bank spec and inlining are incompatible", pair.as_span().start()));
    }
    match pair.as_rule() {
        Rule::id_name => {
            let name = pair.as_str();
            let pair = p.next();
            match pair {
                Some(px) => match px.as_rule() {
                    Rule::block => {
                        let n = name.to_string();
                        if let Some(f) = state.functions.get(&n) {
                            if f.code.is_some() {
                                return Err(syntax_error(state, &format!("Function {} already defined", n), px.as_span().start()));
                            }
                        }
                        let code = compile_block(state, px)?;
                        state.functions.insert(n, Function {
                            order: state.functions.len(),
                            inline,
                            bank,
                            code: Some(code)
                        });
                    },
                    _ => unreachable!(), 
                },
                None => {
                    // This is just a prototype definition
                    let n = name.to_string();
                    if state.functions.get(&n).is_none() {
                        state.functions.insert(n, Function {
                            order: state.functions.len(),
                            inline,
                            bank,
                            code: None 
                        });
                    }
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
            Rule::included_assembler => {
                //debug!("Assembler: {:?}", pair);
                state.included_assembler.push(pair.into_inner().next().unwrap().as_str().to_string());
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
        .op(Op::infix(Rule::assign, Assoc::Right) | Op::infix(Rule::mass, Assoc::Right) | Op::infix(Rule::pass, Assoc::Right) |
            Op::infix(Rule::andass, Assoc::Right) | Op::infix(Rule::orass, Assoc::Right) | Op::infix(Rule::xorass, Assoc::Right) |
            Op::infix(Rule::blsass, Assoc::Right) | Op::infix(Rule::brsass, Assoc::Right))
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
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::not) | Op::prefix(Rule::bnot) | Op::prefix(Rule::mmp) | Op::prefix(Rule::ppp) | Op::prefix(Rule::deref) | Op::prefix(Rule::sizeof))
        .op(Op::postfix(Rule::call) | Op::postfix(Rule::mm) | Op::postfix(Rule::pp));
   
    let calculator = 
        PrattParser::new()
        .op(Op::infix(Rule::lor, Assoc::Left))
        .op(Op::infix(Rule::land, Assoc::Left))
        .op(Op::infix(Rule::or, Assoc::Left))
        .op(Op::infix(Rule::xor, Assoc::Left))
        .op(Op::infix(Rule::and, Assoc::Left))
        .op(Op::infix(Rule::brs, Assoc::Left) | Op::infix(Rule::bls, Assoc::Left))
        .op(Op::infix(Rule::add, Assoc::Left) | Op::infix(Rule::sub, Assoc::Left))
        .op(Op::infix(Rule::mul, Assoc::Left) | Op::infix(Rule::div, Assoc::Left))
        .op(Op::prefix(Rule::neg) | Op::prefix(Rule::not) | Op::prefix(Rule::bnot));

    // Prepare the context
    let mut context = cpp::Context::new(&args.input);
    context.include_directories = args.include_directories.clone();
    context.define("__ATARI2600__", "1");
    for i in &args.defines {
        let mut s = i.splitn(2, '=');
        let def = s.next().unwrap();
        let value = s.next().unwrap_or("1");
        context.define(def, value);
    }

    // Start preprocessor
    //debug!("Preprocessor");
    let mapped_lines = cpp::process(input, &mut preprocessed, &mut context)?;
    //debug!("Mapped lines = {:?}", mapped_lines);

    let preprocessed_utf8 = std::str::from_utf8(&preprocessed)?;
    
    // Prepare the state
    let mut state = CompilerState {
        variables: HashMap::new(),
        functions: HashMap::new(),
        pratt, calculator,
        mapped_lines: &mapped_lines,
        preprocessed_utf8,
        included_assembler: Vec::<String>::new(),
        context,
        signed_chars: args.signed_chars,
        literal_counter: 0,
    };

    let r = Cc2600Parser::parse(Rule::program, preprocessed_utf8);
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

    state.variables.insert("DUMMY".to_string(), Variable {
        order: state.variables.len(),
        signed: false,
        memory: VariableMemory::Zeropage,
        var_const: true,
        alignment: 1,
        def: VariableDefinition::Value(0x2d),
        var_type: VariableType::Char, size: 1});

    // Generate assembly code from compilation output (abstract syntax tree)
    build_cartridge(&state, output, args)?;
    Ok(())
}

