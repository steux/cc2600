use crate::error::Error;
use crate::compile::*;

use std::io::prelude::*;
use std::fs::File;

use log::debug;

struct GeneratorState<'a> {
    last_included_line_number: usize,
    last_included_position: usize,
    last_included_char: std::str::Chars<'a>,
    file: File,
    local_label_counter_for: u32,
    local_label_counter_if: u32,
    loops: Vec<String>,
    flags: FlagsState<'a>,
}

impl<'a> GeneratorState<'a> {
    fn write(&mut self, s: &str) -> Result<usize, std::io::Error> {
        self.file.write(s.as_bytes())
    }
    fn write_asm(&mut self, asm: &str, cycles: u32) -> Result<usize, std::io::Error> {
        self.file.write(format!("\t{:23}\t; {} cycles\n", asm, cycles).as_bytes())
    }
    fn write_label(&mut self, label: &str) -> Result<usize, std::io::Error> {
        self.flags = FlagsState::Unknown;
        self.write(label)?;
        self.write(&"\n")
    }
}

#[derive(Debug, PartialEq)]
enum ExprType<'a> {
    Immediate(i32),
    Absolute(&'a str),
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
    A, X, Y,
    NotRelevant
}

#[derive(Debug, PartialEq, Copy, Clone)]
enum FlagsState<'a> {
    Unknown,
    A, X, Y,
    Zero, Positive, Negative,
    Var((&'a str, Subscript)),
}

struct ExprOutput<'a> {
    t: ExprType<'a>,
    flags: FlagsState<'a>,
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

fn generate_assign<'a>(lhs: &Expr, rhs: &Expr<'a>, state: &State<'a>, gstate: &mut GeneratorState, pos: usize) -> Result<ExprOutput<'a>, Error>
{
    match lhs {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    let expr_output = generate_expr(rhs, state, gstate, pos)?;
                    match expr_output.t {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDX #{}", v), 2)?;
                            Ok(ExprOutput { t: ExprType::X, flags: if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero }})
                        },
                        ExprType::Absolute(name) => {
                            let v = state.get_variable(name);
                            gstate.write_asm(&format!("LDX {}", name), if v.zeropage {3} else {4})?;
                            Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            gstate.write_asm(&"TAX", 2)?;
                            Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDX {},Y", name), 4)?;
                            Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                        },
                        ExprType::A => {
                            gstate.write_asm(&"TAX", 2)?;
                            Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                        },
                        ExprType::X => {
                            Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                        },
                        ExprType::Y => {
                            gstate.write_asm(&"TYA", 2)?;
                            gstate.write_asm(&"TAX", 2)?;
                            Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                        },
                        ExprType::NotRelevant => Err(syntax_error(state, "Bad right side in assignement", pos))
                    }
                },
                "Y" => {
                    let expr_output = generate_expr(rhs, state, gstate, pos)?;
                    match expr_output.t {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDY #{}", v), 2)?;
                            Ok(ExprOutput { t: ExprType::Y, flags: if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero }})
                        },
                        ExprType::Absolute(name) => {
                            let v = state.get_variable(name);
                            gstate.write_asm(&format!("LDY {}", name), if v.zeropage {3} else {4})?;
                            Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDY {},X", name), 4)?;
                            Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            gstate.write_asm(&"TAY", 2)?;
                            Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                        },
                        ExprType::A => {
                            gstate.write_asm(&"TAY", 2)?;
                            Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                        },
                        ExprType::X => {
                            gstate.write_asm(&"TXA", 2)?;
                            gstate.write_asm(&"TAY", 2)?;
                            Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                        },
                        ExprType::Y => {
                            Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                        },
                        ExprType::NotRelevant => Err(syntax_error(state, "Bad right side in assignement", pos))
                    }
                } ,
                variable => {
                    let v = state.get_variable(variable);
                    let cycles = if v.zeropage { 3 } else { 4 };
                    let expr_output = generate_expr(rhs, state, gstate, pos)?;
                    match expr_output.t {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDA #{}", v), 2)?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, flags: if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero }})
                        },
                        ExprType::Absolute(name) => {
                            let v = state.get_variable(name);
                            gstate.write_asm(&format!("LDA {}", name), if v.zeropage {3} else {4})?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, flags: FlagsState::A })
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, flags: FlagsState::A })
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDA {},Y", name), 4)?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, flags: FlagsState::A })
                        },
                        ExprType::A => {
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, flags: expr_output.flags })
                        },
                        ExprType::X => {
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STX {}", variable), cycles)?,
                                Subscript::X => {
                                    gstate.write_asm(&"TXA", 2)?;
                                    gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?
                                },
                                Subscript::Y => if v.zeropage {
                                    gstate.write_asm(&format!("STX {},Y", variable), 4)?
                                } else {
                                    gstate.write_asm(&"TXA", 2)?;
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?
                                },
                            };
                            Ok(ExprOutput { t: ExprType::X, flags: expr_output.flags })
                        },
                        ExprType::Y => {
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STY {}", variable), cycles)?,
                                Subscript::X => if v.zeropage {
                                    gstate.write_asm(&format!("STY {},X", variable), 4)?
                                } else {
                                    gstate.write_asm(&"TYA", 2)?;
                                    gstate.write_asm(&format!("STA {},X", variable), 5)?
                                },
                                Subscript::Y => {
                                    gstate.write_asm(&"TYA", 2)?;
                                    gstate.write_asm(&format!("STA {},Y", variable), 5)?
                                },
                            };
                            Ok(ExprOutput { t: ExprType::Y, flags: expr_output.flags })
                        },
                        ExprType::NotRelevant => Err(syntax_error(state, "Bad right side in assignement", pos))
                    }
                }
            }
        },
        _ => Err(syntax_error(state, "Bad left value in assignement", pos)),
    }
}

fn generate_minusminus<'a>(expr: &Expr<'a>, state: &State<'a>, gstate: &mut GeneratorState, pos: usize) -> Result<ExprOutput<'a>, Error>
{
    match expr {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    gstate.write_asm("DEX", 2)?;
                    Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                },
                "Y" => {
                    gstate.write_asm("DEY", 2)?;
                    Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                },
                variable => {
                    let v = state.get_variable(variable);
                    let cycles = if v.zeropage { 5 } else { 6 };
                    match sub {
                        Subscript::None => {
                            gstate.write_asm(&format!("DEC {}", variable), cycles)?;
                            Ok(ExprOutput { t: ExprType::NotRelevant, flags: FlagsState::Var((var, *sub)) })
                        },
                        Subscript::X => {
                            gstate.write_asm(&format!("DEC {},X", variable), cycles + 1)?;
                            Ok(ExprOutput { t: ExprType::NotRelevant, flags: FlagsState::Var((var, *sub)) })
                        },
                        Subscript::Y => Err(syntax_error(state, "Bad left value used with -- operator (no Y subscript allowed)", pos))
                        
                    }
                }
            }
        },
        _ => Err(syntax_error(state, "Bad left value used with -- operator", pos)),
    }
}

fn generate_plusplus<'a>(expr: &Expr<'a>, state: &State<'a>, gstate: &mut GeneratorState, pos: usize) -> Result<ExprOutput<'a>, Error>
{
    match expr {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    gstate.write_asm("INX", 2)?;
                    Ok(ExprOutput { t: ExprType::X, flags: FlagsState::X })
                },
                "Y" => {
                    gstate.write_asm("INY", 2)?;
                    Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Y })
                },
                variable => {
                    let v = state.get_variable(variable);
                    let cycles = if v.zeropage { 5 } else { 6 };
                    match sub {
                        Subscript::None => {
                            gstate.write_asm(&format!("INC {}", variable), cycles)?;
                            Ok(ExprOutput { t: ExprType::NotRelevant, flags: FlagsState::Var((var, *sub)) })
                        },
                        Subscript::X => {
                            gstate.write_asm(&format!("INC {},X", variable), cycles + 1)?;
                            Ok(ExprOutput { t: ExprType::NotRelevant, flags: FlagsState::Var((var, *sub)) })
                        },
                        Subscript::Y => Err(syntax_error(state, "Bad left value used with ++ operator (no Y subscript allowed)", pos))
                    }
                }
            }
        },
        _ => Err(syntax_error(state, "Bad left value used with ++ operator", pos)),
    }
}

fn generate_expr<'a>(expr: &Expr<'a>, state: &State<'a>, gstate: &mut GeneratorState, pos: usize) -> Result<ExprOutput<'a>, Error>
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
        Expr::Integer(i) => Ok(ExprOutput { t: ExprType::Immediate(*i), flags: FlagsState::Unknown }),
        Expr::BinOp {lhs, op, rhs} => {
            match op {
                Operation::Assign => generate_assign(lhs, rhs, state, gstate, pos),
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
                "X" => Ok(ExprOutput { t: ExprType::X, flags: FlagsState::Unknown }),
                "Y" => Ok(ExprOutput { t: ExprType::Y, flags: FlagsState::Unknown }),
                variable => match sub {
                    Subscript::None => Ok(ExprOutput { t: ExprType::Absolute(variable), flags: FlagsState::Unknown }),
                    Subscript::X => Ok(ExprOutput { t: ExprType::AbsoluteX(variable), flags: FlagsState::Unknown }),
                    Subscript::Y => Ok(ExprOutput { t: ExprType::AbsoluteY(variable), flags: FlagsState::Unknown }),
                }
            }
        },
        Expr::MinusMinus(v) => generate_minusminus(v, state, gstate, pos),
        Expr::PlusPlus(v) => generate_plusplus(v, state, gstate, pos),
        _ => unreachable!() 
    }
}

fn generate_condition(condition: &Expr, state: &State, gstate: &mut GeneratorState, pos: usize, negate: bool, label: &str) -> Result<(), Error>
{
    debug!("Condition: {:?}", condition);
    match condition {
        Expr::BinOp {lhs, op, rhs} => {
            match op {
                Operation::Eq | Operation::Neq => {
                    let left = generate_expr(lhs, state, gstate, pos)?;
                    match left.t {
                        ExprType::X | ExprType::Y => {
                            // This condition is using the special index registers
                            let right = generate_expr(rhs, state, gstate, pos)?;
                            match right.t {
                                ExprType::Immediate(v) => {
                                    // Special case: compare with 0
                                    if v == 0 {
                                        // Let's see if we can shortcut CPX or CPY
                                        let flags_ok = (gstate.flags == FlagsState::X && left.t == ExprType::X) || (gstate.flags == FlagsState::Y && left.t == ExprType::Y);
                                        if flags_ok {
                                            if negate {
                                                match op {
                                                    Operation::Eq => {
                                                        gstate.write_asm(&format!("BNE {}", label), 2)?;
                                                        Ok(())
                                                    },
                                                    Operation::Neq => {
                                                        gstate.write_asm(&format!("BEQ {}", label), 2)?;
                                                        Ok(())
                                                    },
                                                    _ => unreachable!() 
                                                }
                                            } else {
                                                match op {
                                                    Operation::Eq => {
                                                        gstate.write_asm(&format!("BEQ {}", label), 2)?;
                                                        Ok(())
                                                    },
                                                    Operation::Neq => {
                                                        gstate.write_asm(&format!("BNE {}", label), 2)?;
                                                        Ok(())
                                                    },
                                                    _ => unreachable!() 
                                                }
                                            }
                                        }
                                        else { Err(Error::Unimplemented { feature: "for loop statement is partially implemented" }) } 
                                    } else { Err(Error::Unimplemented { feature: "for loop statement is partially implemented" }) }
                                },
                                _ => Err(Error::Unimplemented { feature: "for loop statement is partially implemented" })
                            }
                        },
                        _ => Err(Error::Unimplemented { feature: "for loop statement is partially implemented" })
                    }
                },
                _ => Err(Error::Unimplemented { feature: "for loop statement is partially implemented" })
            }
        },
        _ => Err(Error::Unimplemented { feature: "for loop statement is partially implemented" })
    }
}

fn generate_for_loop<'a>(init: &Expr<'a>, condition: &Expr, update: &Expr<'a>, body: &StatementLoc<'a>, state: &State<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    gstate.local_label_counter_for += 1;
    let forloop_label = format!(".forloop{}", gstate.local_label_counter_for);
    let forloopend_label = format!(".forloopend{}", gstate.local_label_counter_for);
    gstate.flags = generate_expr(init, state, gstate, pos)?.flags;
    gstate.loops.push(forloopend_label.clone());
    generate_condition(condition, state, gstate, pos, true, &forloopend_label)?;
    gstate.write_label(&forloop_label)?;
    generate_statement(body, state, gstate)?;
    gstate.flags = generate_expr(update, state, gstate, pos)?.flags;
    generate_condition(condition, state, gstate, pos, false, &forloop_label)?;
    gstate.write_label(&forloopend_label)?;
    gstate.loops.pop();
    Ok(())
}

fn generate_statement<'a>(code: &StatementLoc<'a>, state: &State<'a>, gstate: &mut GeneratorState<'a>) -> Result<FlagsState<'a>, Error>
{
    // Generate different kind of statements
    match &code.statement {
        Statement::Block(statements) => {
            for code in statements {
                gstate.flags = generate_statement(&code, state, gstate)?;
            }
            Ok(gstate.flags)
        },
        Statement::Expression(expr) => { 
            gstate.flags = generate_expr(&expr, state, gstate, code.pos)?.flags; 
            Ok(gstate.flags) 
        },
        Statement::For { init, condition, update, body } => { 
            generate_for_loop(init, condition, update, body.as_ref(), state, gstate, code.pos)?; 
            gstate.flags = FlagsState::Unknown;
            Ok(gstate.flags) 
        },
        Statement::If { condition, body, else_body } => Err(Error::Unimplemented { feature: "if statement not implemented" }),
        Statement::Break => Err(Error::Unimplemented { feature: "break statement not implemented" }),
        Statement::Continue => Err(Error::Unimplemented { feature: "continue statement not implemented" }),
    }
}

pub fn generate_asm(state: &State, filename: &str) -> Result<(), Error> 
{
    let file = File::create(filename)?;

    let mut gstate = GeneratorState {
        last_included_line_number: 0,
        last_included_position: 0,
        last_included_char: state.preprocessed_utf8.chars(),
        file,
        local_label_counter_for: 0,
        local_label_counter_if: 0,
        loops: Vec::new(),
        flags: FlagsState::Unknown,
    };

    gstate.write("\tPROCESSOR 6502\n")?;
    gstate.write("\tSEG.U variables\n\tORG $80\n\n")?;
    
    // Generate vaiables code
    for v in state.sorted_variables().iter() {
        gstate.file.write_all(format!("{:23}\tds {}\n", v.0, v.1.size).as_bytes())?; 
    }

    gstate.write("\n\tSEG.U code\n\tORG $F000\n")?;

    // Generate functions code
    gstate.write("\n; Functions definitions\n")?;
    for f in state.sorted_functions().iter() {
        gstate.write(&format!("\n{}\tSUBROUTINE\n", f.0))?;
        gstate.local_label_counter_for = 0;
        gstate.local_label_counter_if = 0;
        generate_statement(&f.1.code, state, &mut gstate)?;
        gstate.write_asm("RTS", 6)?;
    }

    Ok(())
}
