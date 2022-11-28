use crate::error::Error;
use crate::compile::*;

use std::io::prelude::*;
use std::fs::File;

use log::debug;

struct GeneratorState<'a> {
    compiler_state: &'a CompilerState<'a>,
    last_included_line_number: usize,
    last_included_position: usize,
    last_included_char: std::str::Chars<'a>,
    file: File,
    local_label_counter_for: u32,
    local_label_counter_if: u32,
    loops: Vec<(String,String)>,
    flags: FlagsState<'a>,
    acc_in_use: bool,
}

impl<'a> GeneratorState<'a> {
    fn write(&mut self, s: &str) -> Result<usize, std::io::Error> {
        self.file.write(s.as_bytes())
    }
    fn write_asm(&mut self, asm: &str, cycles: u32) -> Result<usize, std::io::Error> {
        self.file.write(format!("\t{:23}\t; {} cycles\n", asm, cycles).as_bytes())
    }
    fn write_label(&mut self, label: &str) -> Result<usize, std::io::Error> {
        self.flags = FlagsState::Unknown; // There is a label, so there are some jumps to it -
                                          // flags are then unknown at that point
        self.write(label)?;
        self.write(&"\n")
    }
}

#[derive(Debug, PartialEq)]
enum ExprType<'a> {
    Nothing,
    Immediate(i32),
    Absolute(&'a str),
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
    A, X, Y,
}

#[derive(Debug, PartialEq, Copy, Clone)]
enum FlagsState<'a> {
    Unknown,
    X, Y,
    Zero, Positive, Negative,
    Var((&'a str, Subscript)),
}

fn generate_included_source_code_line<'a>(loc: usize, gstate: &'a mut GeneratorState) -> Option<&'a str>
{
    let mut start_of_line = gstate.last_included_char.clone();
    let mut start_of_line_pos = gstate.last_included_position;
    if gstate.last_included_position < loc {
        // Let's find the line including loc
        while gstate.last_included_position < loc {
            let c = gstate.last_included_char.next();
            if c.is_none() { return None; }
            let c = c.unwrap();
            gstate.last_included_position += 1;
            if c == '\n' { 
                gstate.last_included_line_number += 1;
                start_of_line = gstate.last_included_char.clone();
                start_of_line_pos = gstate.last_included_position;
            }
        };
        // Ok, we have found loc. Let's go to the end of line
        loop {
            let c = gstate.last_included_char.next();
            if c.is_none() { return Some(start_of_line.as_str()); }
            let c = c.unwrap();
            gstate.last_included_position += 1;
            if c == '\n' {
                gstate.last_included_line_number += 1;
                return Some(&start_of_line.as_str()[0..(gstate.last_included_position - start_of_line_pos)]);
            }
        }    
    }
    None
}

fn generate_assign<'a>(lhs: &Expr<'a>, rhs: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match lhs {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    let expr_output = generate_expr(rhs, gstate, pos)?;
                    match expr_output {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDX #{}", v), 2)?;
                            gstate.flags = if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero };
                            Ok(ExprType::X) 
                        },
                        ExprType::Absolute(name) => {
                            let v = gstate.compiler_state.get_variable(name);
                            gstate.write_asm(&format!("LDX {}", name), if v.zeropage {3} else {4})?;
                            gstate.flags = FlagsState::X;
                            Ok(ExprType::X)
                        },
                        ExprType::AbsoluteX(name) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            gstate.write_asm(&"TAX", 2)?;
                            if gstate.acc_in_use { 
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::X;
                            }
                            Ok(ExprType::X)
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDX {},Y", name), 4)?;
                            gstate.flags = FlagsState::X;
                            Ok(ExprType::X)
                        },
                        ExprType::A => {
                            gstate.write_asm(&"TAX", 2)?;
                            gstate.flags = FlagsState::X;
                            Ok(ExprType::X)
                        },
                        ExprType::X => {
                            Ok(ExprType::X)
                        },
                        ExprType::Y => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&"TYA", 2)?;
                            gstate.write_asm(&"TAX", 2)?;
                            if gstate.acc_in_use { 
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::X;
                            }
                            Ok(ExprType::X)
                        },
                        ExprType::Nothing => unreachable!()
                    }
                },
                "Y" => {
                    let expr_output = generate_expr(rhs, gstate, pos)?;
                    match expr_output {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDY #{}", v), 2)?;
                            gstate.flags = if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero };
                            Ok(ExprType::Y) 
                        },
                        ExprType::Absolute(name) => {
                            let v = gstate.compiler_state.get_variable(name);
                            gstate.write_asm(&format!("LDY {}", name), if v.zeropage {3} else {4})?;
                            gstate.flags = FlagsState::Y;
                            Ok(ExprType::Y)
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDY {},X", name), 4)?;
                            gstate.flags = FlagsState::Y;
                            Ok(ExprType::Y)
                        },
                        ExprType::AbsoluteY(name) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            gstate.write_asm(&"TAY", 2)?;
                            if gstate.acc_in_use { 
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::Y;
                            }
                            Ok(ExprType::Y)
                        },
                        ExprType::A => {
                            gstate.write_asm(&"TAY", 2)?;
                            gstate.flags = FlagsState::Y;
                            Ok(ExprType::Y)
                        },
                        ExprType::X => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&"TXA", 2)?;
                            gstate.write_asm(&"TAY", 2)?;
                            if gstate.acc_in_use { 
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::Y;
                            }
                            Ok(ExprType::Y)
                        },
                        ExprType::Y => {
                            gstate.flags = FlagsState::Y;
                            Ok(ExprType::Y)
                        },
                        ExprType::Nothing => unreachable!()
                    }
                },
                variable => {
                    let v = gstate.compiler_state.get_variable(variable);
                    let cycles = if v.zeropage { 3 } else { 4 };
                    let expr_output = generate_expr(rhs, gstate, pos)?;
                    match expr_output {
                        ExprType::Immediate(v) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&format!("LDA #{}", v), 2)?;
                            match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STA {}", variable), cycles)?;
                                },
                                Subscript::X => {
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                                },
                                Subscript::Y => {
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                }
                            };
                            if gstate.acc_in_use { 
                                gstate.write_asm("PLA", 3)?; 
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero };
                            }
                            Ok(ExprType::Immediate(v))
                        },
                        ExprType::Absolute(name) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            let v = gstate.compiler_state.get_variable(name);
                            gstate.write_asm(&format!("LDA {}", name), if v.zeropage {3} else {4})?;
                            match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STA {}", variable), cycles)?;
                                },
                                Subscript::X => {
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                                },
                                Subscript::Y => {
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                }
                            };
                            if gstate.acc_in_use {
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::Var((name, Subscript::None));
                            }
                            Ok(ExprType::Absolute(name))
                        },
                        ExprType::AbsoluteX(name) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STA {}", variable), cycles)?;
                                },
                                Subscript::X => {
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                                },
                                Subscript::Y => {
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                }
                            };
                            if gstate.acc_in_use {
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::Var((name, Subscript::X));
                            }
                            Ok(ExprType::AbsoluteX(name))
                        },
                        ExprType::AbsoluteY(name) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&format!("LDA {},Y", name), 4)?;
                            match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STA {}", variable), cycles)?;
                                },
                                Subscript::X => {
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                                },
                                Subscript::Y => {
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                }
                            };
                            if gstate.acc_in_use {
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::Var((name, Subscript::Y));
                            }
                            Ok(ExprType::AbsoluteY(name))
                        },
                        ExprType::A => {
                            let e = match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STA {}", variable), cycles)?;
                                    ExprType::Absolute(variable)
                                },
                                Subscript::X => {
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                                    ExprType::AbsoluteX(variable)
                                },
                                Subscript::Y => {
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                    ExprType::AbsoluteY(variable)
                                }
                            };
                            Ok(e)
                        },
                        ExprType::X => {
                            match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STX {}", variable), cycles)?;
                                },
                                Subscript::X => {
                                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                                    gstate.write_asm(&"TXA", 2)?;
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                                    if gstate.acc_in_use {
                                        gstate.write_asm("PLA", 3)?;
                                        gstate.flags = FlagsState::Unknown;
                                    } else {
                                        gstate.flags = FlagsState::X;
                                    }
                                },
                                Subscript::Y => if v.zeropage {
                                    gstate.write_asm(&format!("STX {},Y", variable), 4)?;
                                } else {
                                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                                    gstate.write_asm(&"TXA", 2)?;
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                    if gstate.acc_in_use {
                                        gstate.write_asm("PLA", 3)?;
                                        gstate.flags = FlagsState::Unknown;
                                    } else {
                                        gstate.flags = FlagsState::X;
                                    }
                                },
                            };
                            Ok(ExprType::X)
                        },
                        ExprType::Y => {
                            match sub {
                                Subscript::None => {
                                    gstate.write_asm(&format!("STY {}", variable), cycles)?;
                                },
                                Subscript::X => if v.zeropage {
                                    gstate.write_asm(&format!("STY {},X", variable), 4)?;
                                } else {
                                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                                    gstate.write_asm(&"TYA", 2)?;
                                    gstate.write_asm(&format!("STA {},X", variable), 5)?;
                                    if gstate.acc_in_use {
                                        gstate.write_asm("PLA", 3)?;
                                        gstate.flags = FlagsState::Unknown;
                                    } else {
                                        gstate.flags = FlagsState::Y;
                                    }
                                },
                                Subscript::Y => {
                                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                                    gstate.write_asm(&"TYA", 2)?;
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                                    if gstate.acc_in_use {
                                        gstate.write_asm("PLA", 3)?;
                                        gstate.flags = FlagsState::Unknown;
                                    } else {
                                        gstate.flags = FlagsState::Y;
                                    }
                                },
                            };
                            Ok(ExprType::Y)
                        },
                        ExprType::Nothing => unreachable!()
                    }
                }
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
    }
}

fn generate_minusminus<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    gstate.write_asm("DEX", 2)?;
                    gstate.flags = FlagsState::X;
                    Ok(ExprType::X)
                },
                "Y" => {
                    gstate.write_asm("DEY", 2)?;
                    gstate.flags = FlagsState::Y;
                    Ok(ExprType::Y)
                },
                variable => {
                    let v = gstate.compiler_state.get_variable(variable);
                    let cycles = if v.zeropage { 5 } else { 6 };
                    match sub {
                        Subscript::None => {
                            gstate.write_asm(&format!("DEC {}", variable), cycles)?;
                            gstate.flags = FlagsState::Var((var, *sub));
                            Ok(ExprType::Absolute(variable))
                        },
                        Subscript::X => {
                            gstate.write_asm(&format!("DEC {},X", variable), cycles + 1)?;
                            gstate.flags = FlagsState::Var((var, *sub));
                            Ok(ExprType::AbsoluteX(variable))
                        },
                        Subscript::Y => Err(syntax_error(gstate.compiler_state, "Bad left value used with ++ operator (no Y subscript allowed)", pos))
                    }
                }
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Bad left value used with ++ operator", pos)),
    }
}

fn generate_plusplus<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    gstate.write_asm("INX", 2)?;
                    gstate.flags = FlagsState::X;
                    Ok(ExprType::X)
                },
                "Y" => {
                    gstate.write_asm("INY", 2)?;
                    gstate.flags = FlagsState::Y;
                    Ok(ExprType::Y)
                },
                variable => {
                    let v = gstate.compiler_state.get_variable(variable);
                    let cycles = if v.zeropage { 5 } else { 6 };
                    match sub {
                        Subscript::None => {
                            gstate.write_asm(&format!("INC {}", variable), cycles)?;
                            gstate.flags = FlagsState::Var((var, *sub));
                            Ok(ExprType::Absolute(variable))
                        },
                        Subscript::X => {
                            gstate.write_asm(&format!("INC {},X", variable), cycles + 1)?;
                            gstate.flags = FlagsState::Var((var, *sub));
                            Ok(ExprType::AbsoluteX(variable))
                        },
                        Subscript::Y => Err(syntax_error(gstate.compiler_state, "Bad left value used with ++ operator (no Y subscript allowed)", pos))
                    }
                }
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Bad left value used with ++ operator", pos)),
    }
}

fn generate_neg<'a>(expr: &Expr<'a>, _gstate: &mut GeneratorState<'a>, _pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(-*i)),
        _ => { return Err(Error::Unimplemented { feature: "negation statement is partially implemented" }); },
    }
}

fn generate_expr<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    // Include C source code into generated asm
    // debug!("{:?}, {}, {}, {}", expr, pos, gstate.last_included_position, gstate.last_included_line_number);
    let included_source_code = generate_included_source_code_line(pos, gstate);
    let line_to_be_written = if let Some(line) = included_source_code {
        Some(line.to_string())
    } else {
        None
    };
    // debug!("{:?}, {}, {}", line_to_be_written, gstate.last_included_position, gstate.last_included_line_number);
    if let Some(l) = line_to_be_written {
        let f = &mut gstate.file; 
        f.write_all(b";")?;
        f.write_all(l.as_bytes())?; // Should include the '\n'
    }

    debug!("Expression: {:?}", expr);
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(*i)),
        Expr::BinOp {lhs, op, rhs} => {
            match op {
                Operation::Assign => generate_assign(lhs, rhs, gstate, pos),
                Operation::Add => Err(Error::Unimplemented { feature: "Addition not implemented" }),
                Operation::Sub => Err(Error::Unimplemented { feature: "Subtraction not implemented" }),
                Operation::Eq => Err(Error::Unimplemented { feature: "Equal not implemented" }),
                Operation::Neq => Err(Error::Unimplemented { feature: "Not equal not implemented" }),
                Operation::Gt => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
                Operation::Gte => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
                Operation::Lt => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
                Operation::Lte => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
            }
        },
        Expr::Var((var, sub)) => {
            match *var {
                "X" => Ok(ExprType::X),
                "Y" => Ok(ExprType::Y),
                variable => match sub {
                    Subscript::None => Ok(ExprType::Absolute(variable)),
                    Subscript::X => Ok(ExprType::AbsoluteX(variable)),
                    Subscript::Y => Ok(ExprType::AbsoluteY(variable)),
                }
            }
        },
        Expr::MinusMinus(v) => generate_minusminus(v, gstate, pos),
        Expr::PlusPlus(v) => generate_plusplus(v, gstate, pos),
        Expr::Neg(v) => generate_neg(v, gstate, pos),
        Expr::Nothing => Ok(ExprType::Nothing),
    }
}

fn flags_ok(flags: &FlagsState, expr_type: &ExprType) -> bool
{
    match flags {
        FlagsState::X => *expr_type == ExprType::X,
        FlagsState::Y => *expr_type == ExprType::Y,
        FlagsState::Var((s, sub)) => match sub {
            Subscript::None => *expr_type == ExprType::Absolute(s),
            Subscript::X => *expr_type == ExprType::AbsoluteX(s),
            Subscript::Y => *expr_type == ExprType::AbsoluteY(s),
        },
        _ => false
    }
}

fn generate_condition<'a>(condition: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize, negate: bool, label: &str) -> Result<(), Error>
{
    debug!("Condition: {:?}", condition);
    match condition {
        Expr::BinOp {lhs, op, rhs} => {
            let left = generate_expr(lhs, gstate, pos)?;
            let right = generate_expr(rhs, gstate, pos)?;
    
            let operator = if negate {
                match op {
                    Operation::Eq => Operation::Neq,
                    Operation::Neq => Operation::Eq,
                    Operation::Gt => Operation::Lte,
                    Operation::Gte => Operation::Lt,
                    Operation::Lt => Operation::Gte,
                    Operation::Lte => Operation::Lt,
                    _ => unreachable!()
                }
            } else { *op };
           
            if let ExprType::Immediate(v) = right {
                if v == 0 {
                // Let's see if we can shortcut compare instruction 
                    if flags_ok(&gstate.flags, &left) {
                        match operator {
                            Operation::Neq => {
                                gstate.write_asm(&format!("BNE {}", label), 2)?;
                                return Ok(());
                            },
                            Operation::Eq => {
                                gstate.write_asm(&format!("BEQ {}", label), 2)?;
                                return Ok(());
                            },
                            _ => unreachable!(),
                        }
                    } 
                }
            }

            // Compare instruction
            let signed;
            let cmp;
            match left {
                ExprType::Absolute(varname) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    let v = gstate.compiler_state.get_variable(varname);
                    signed = v.var_type == VariableType::SignedChar;
                    let cycles = if v.zeropage { 3 } else { 4 };
                    gstate.write_asm(&format!("LDA {}", varname), cycles)?;
                    cmp = true;
                },
                ExprType::AbsoluteX(varname) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    let v = gstate.compiler_state.get_variable(varname);
                    signed = v.var_type == VariableType::SignedChar;
                    gstate.write_asm(&format!("LDA {},X", varname), 4)?;
                    cmp = true;
                },
                ExprType::AbsoluteY(varname) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    let v = gstate.compiler_state.get_variable(varname);
                    signed = v.var_type == VariableType::SignedChar;
                    gstate.write_asm(&format!("LDA {},Y", varname), 4)?;
                    cmp = true;
                },
                ExprType::Y => {
                    signed = false;
                    match right {
                        ExprType::Absolute(varname) => {
                            let v = gstate.compiler_state.get_variable(varname);
                            let cycles = if v.zeropage { 3 } else { 4 };
                            gstate.write_asm(&format!("CPY {}", varname), cycles)?;
                            cmp = false;
                        },
                        _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
                    } 
                },
                _ => { return Err(Error::Unimplemented { feature: "condition statement is partially implemented" }); },
            }

            if cmp {
                match right {
                    ExprType::Immediate(v) => {
                        gstate.write_asm(&format!("CMP #{}", v), 2)?;
                    },
                    ExprType::Absolute(name) => {
                        let v = gstate.compiler_state.get_variable(name);
                        let cycles = if v.zeropage { 3 } else { 4 };
                        gstate.write_asm(&format!("CMP {}", name), cycles)?;
                    },
                    ExprType::AbsoluteX(name) => {
                        gstate.write_asm(&format!("CMP {},X", name), 4)?;
                    },
                    ExprType::AbsoluteY(name) => {
                        gstate.write_asm(&format!("CMP {},Y", name), 4)?;
                    },
                    _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
                } 
                if gstate.acc_in_use { 
                    gstate.write_asm("PLA", 3)?; 
                }
            }

            gstate.flags = FlagsState::Unknown;
            
            // Branch instruction
            match operator {
                Operation::Neq => {
                    gstate.write_asm(&format!("BNE {}", label), 2)?;
                    return Ok(());
                },
                Operation::Eq => {
                    gstate.write_asm(&format!("BEQ {}", label), 2)?;
                    return Ok(());
                },
                Operation::Lt => {
                    if signed {
                        gstate.write_asm(&format!("BMI {}", label), 2)?;
                    } else {
                        gstate.write_asm(&format!("BCC {}", label), 2)?;
                    } 
                    Ok(())
                },
                Operation::Gt => {
                    let label_here = format!(".ifhere{}", gstate.local_label_counter_if);
                    if signed {
                        gstate.write_asm(&format!("BEQ {}", label_here), 2)?;
                        gstate.write_asm(&format!("BPL {}", label), 2)?;
                    } else {
                        gstate.write_asm(&format!("BEQ {}", label_here), 2)?;
                        gstate.write_asm(&format!("BCS {}", label), 2)?;
                    }
                    gstate.write_label(&label_here)?;
                    Ok(())
                },
                Operation::Lte => {
                    if signed {
                        gstate.write_asm(&format!("BMI {}", label), 2)?;
                        gstate.write_asm(&format!("BEQ {}", label), 2)?;
                    } else {
                        gstate.write_asm(&format!("BCC {}", label), 2)?;
                        gstate.write_asm(&format!("BEQ {}", label), 2)?;
                    } 
                    Ok(())
                },
                Operation::Gte => {
                    if signed {
                        gstate.write_asm(&format!("BPL {}", label), 2)?;
                    } else {
                        gstate.write_asm(&format!("BCS {}", label), 2)?;
                    } 
                    Ok(())
                },
                _ => Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
            }
        },
        _ => Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
    }
}

fn generate_for_loop<'a>(init: &Expr<'a>, condition: &Expr<'a>, update: &Expr<'a>, body: &StatementLoc<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    gstate.local_label_counter_for += 1;
    let for_label = format!(".for{}", gstate.local_label_counter_for);
    let forupdate_label = format!(".forupdate{}", gstate.local_label_counter_for);
    let forend_label = format!(".forend{}", gstate.local_label_counter_for);
    generate_expr(init, gstate, pos)?;
    gstate.loops.push((forupdate_label.clone(), forend_label.clone()));
    generate_condition(condition, gstate, pos, true, &forend_label)?;
    gstate.write_label(&for_label)?;
    generate_statement(body, gstate)?;
    gstate.write_label(&forupdate_label)?;
    generate_expr(update, gstate, pos)?;
    generate_condition(condition, gstate, pos, false, &for_label)?;
    gstate.write_label(&forend_label)?;
    gstate.loops.pop();
    Ok(())
}

fn generate_if<'a>(condition: &Expr<'a>, body: &StatementLoc<'a>, else_body: Option<&StatementLoc<'a>>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    gstate.local_label_counter_if += 1;
    let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
    match else_body {
        None => {
            match body.statement {
                Statement::Break => {
                    let brk_label = {
                        match gstate.loops.last() {
                            None => return Err(syntax_error(gstate.compiler_state, "Break statement outside loop", pos)),
                            Some((_, bl)) => bl.clone(),
                        }
                    };
                    generate_condition(condition, gstate, pos, false, &brk_label)?;
                },
                Statement::Continue => {
                    let cont_label = {
                        match gstate.loops.last() {
                            None => return Err(syntax_error(gstate.compiler_state, "Break statement outside loop", pos)),
                            Some((cl, _)) => cl.clone(),
                        }
                    };
                    generate_condition(condition, gstate, pos, false, &cont_label)?;
                },
                _ => {
                    generate_condition(condition, gstate, pos, true, &ifend_label)?;
                    generate_statement(body, gstate)?;
                    gstate.write_label(&ifend_label)?;
                }

            }
        },
        Some(else_statement) => {
            let else_label = format!(".else{}", gstate.local_label_counter_if);
            generate_condition(condition, gstate, pos, true, &else_label)?;
            let saved_flags = gstate.flags;
            generate_statement(body, gstate)?;
            gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
            gstate.write_label(&else_label)?;
            gstate.flags = saved_flags;
            generate_statement(else_statement, gstate)?;
            gstate.write_label(&ifend_label)?;
        }
    };
    Ok(())
}

fn generate_break<'a>(gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    let brk_label = match gstate.loops.last() {
        None => return Err(syntax_error(gstate.compiler_state, "Break statement outside loop", pos)),
        Some((_, bl)) => bl,
    };
    gstate.write_asm(&format!("JMP {}", brk_label), 3)?;
    Ok(())
}

fn generate_continue<'a>(gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    let cont_label = match gstate.loops.last() {
        None => return Err(syntax_error(gstate.compiler_state, "Break statement outside loop", pos)),
        Some((cl, _)) => cl,
    };
    gstate.write_asm(&format!("JMP {}", cont_label), 3)?;
    Ok(())
}

fn generate_return<'a>(gstate: &mut GeneratorState<'a>) -> Result<(), Error>
{
    gstate.write_asm("RTS", 6)?;
    Ok(())
}

fn generate_statement<'a>(code: &StatementLoc<'a>, gstate: &mut GeneratorState<'a>) -> Result<(), Error>
{
    gstate.acc_in_use = false;
    // Generate different kind of statements
    match &code.statement {
        Statement::Block(statements) => {
            for code in statements {
                generate_statement(&code, gstate)?;
            }
        },
        Statement::Expression(expr) => { 
            generate_expr(&expr, gstate, code.pos)?;
        },
        Statement::For { init, condition, update, body } => { 
            generate_for_loop(init, condition, update, body.as_ref(), gstate, code.pos)?; 
        },
        Statement::If { condition, body, else_body } => { 
            match else_body {
                None => generate_if(condition, body.as_ref(), None, gstate, code.pos)?,
                Some(ebody) => generate_if(condition, body.as_ref(), Some(ebody.as_ref()), gstate, code.pos)?,
            }; 
        },
        Statement::Break => { generate_break(gstate, code.pos)?; }
        Statement::Continue => { generate_continue(gstate, code.pos)?; }
        Statement::Return => { generate_return(gstate)?; }
    }
    Ok(())
}

pub fn generate_asm(compiler_state: &CompilerState, filename: &str) -> Result<(), Error> 
{
    let file = File::create(filename)?;

    let mut gstate = GeneratorState {
        compiler_state,
        last_included_line_number: 0,
        last_included_position: 0,
        last_included_char: compiler_state.preprocessed_utf8.chars(),
        file,
        local_label_counter_for: 0,
        local_label_counter_if: 0,
        loops: Vec::new(),
        flags: FlagsState::Unknown,
        acc_in_use: false,
    };

    gstate.write("\tPROCESSOR 6502\n")?;
    gstate.write("\tSEG.U variables\n\tORG $80\n\n")?;
    
    // Generate vaiables code
    for v in compiler_state.sorted_variables().iter() {
        gstate.file.write_all(format!("{:23}\tds {}\n", v.0, v.1.size).as_bytes())?; 
    }

    gstate.write("\n\tSEG.U code\n\tORG $F000\n")?;

    // Generate functions code
    gstate.write("\n; Functions definitions\n")?;
    for f in compiler_state.sorted_functions().iter() {
        gstate.write(&format!("\n{}\tSUBROUTINE\n", f.0))?;
        gstate.local_label_counter_for = 0;
        gstate.local_label_counter_if = 0;
        generate_statement(&f.1.code, &mut gstate)?;
        gstate.write_asm("RTS", 6)?;
    }

    Ok(())
}
