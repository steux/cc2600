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
}

impl<'a> GeneratorState<'a> {
    fn write(&mut self, s: &str) -> Result<usize, std::io::Error> {
        self.file.write(s.as_bytes())
    }
    fn write_asm(&mut self, asm: &str, cycles: u32) -> Result<usize, std::io::Error> {
        self.file.write(format!("\t{:23}\t; {} cycles\n", asm, cycles).as_bytes())
    }
}

enum ExprType<'a> {
    Immediate(i32),
    Absolute(&'a str),
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
    A, X, Y,
}

enum FlagsState {
    Unknown,
    A, X, Y,
    Zero, Positive, Negative 
}

struct ExprOutput<'a> {
    t: ExprType<'a>,
    zero: FlagsState,
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
                            Ok(ExprOutput { t: ExprType::X, zero: if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero }})
                        },
                        ExprType::Absolute(name) => {
                            let v = state.get_variable(name);
                            gstate.write_asm(&format!("LDX {}", name), if v.zeropage {3} else {4})?;
                            Ok(ExprOutput { t: ExprType::X, zero: FlagsState::X })
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            gstate.write_asm(&"TAX", 2)?;
                            Ok(ExprOutput { t: ExprType::X, zero: FlagsState::X })
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDX {},Y", name), 4)?;
                            Ok(ExprOutput { t: ExprType::X, zero: FlagsState::X })
                        },
                        ExprType::A => {
                            gstate.write_asm(&"TAX", 2)?;
                            Ok(ExprOutput { t: ExprType::X, zero: FlagsState::X })
                        },
                        ExprType::X => {
                            Ok(ExprOutput { t: ExprType::X, zero: FlagsState::X })
                        },
                        ExprType::Y => {
                            gstate.write_asm(&"TYA", 2)?;
                            gstate.write_asm(&"TAX", 2)?;
                            Ok(ExprOutput { t: ExprType::X, zero: FlagsState::X })
                        },
                    }
                },
                "Y" => {
                    let expr_output = generate_expr(rhs, state, gstate, pos)?;
                    match expr_output.t {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDY #{}", v), 2)?;
                            Ok(ExprOutput { t: ExprType::Y, zero: if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero }})
                        },
                        ExprType::Absolute(name) => {
                            let v = state.get_variable(name);
                            gstate.write_asm(&format!("LDY {}", name), if v.zeropage {3} else {4})?;
                            Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Y })
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDY {},X", name), 4)?;
                            Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Y })
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            gstate.write_asm(&"TAY", 2)?;
                            Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Y })
                        },
                        ExprType::A => {
                            gstate.write_asm(&"TAY", 2)?;
                            Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Y })
                        },
                        ExprType::X => {
                            gstate.write_asm(&"TXA", 2)?;
                            gstate.write_asm(&"TAY", 2)?;
                            Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Y })
                        },
                        ExprType::Y => {
                            Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Y })
                        },
                    }
                },
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
                            Ok(ExprOutput { t: ExprType::A, zero: if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero }})
                        },
                        ExprType::Absolute(name) => {
                            let v = state.get_variable(name);
                            gstate.write_asm(&format!("LDA {}", name), if v.zeropage {3} else {4})?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, zero: FlagsState::A })
                        },
                        ExprType::AbsoluteX(name) => {
                            gstate.write_asm(&format!("LDA {},X", name), 4)?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, zero: FlagsState::A })
                        },
                        ExprType::AbsoluteY(name) => {
                            gstate.write_asm(&format!("LDA {},Y", name), 4)?;
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, zero: FlagsState::A })
                        },
                        ExprType::A => {
                            match sub {
                                Subscript::None => gstate.write_asm(&format!("STA {}", variable), cycles)?,
                                Subscript::X => gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?,
                                Subscript::Y => gstate.write_asm(&format!("STA {},Y", variable), 5)?,
                            };
                            Ok(ExprOutput { t: ExprType::A, zero: expr_output.zero })
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
                            Ok(ExprOutput { t: ExprType::X, zero: expr_output.zero })
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
                            Ok(ExprOutput { t: ExprType::Y, zero: expr_output.zero })
                        },
                    }
                }
            }
        },
        _ => Err(syntax_error(state, "Bad left value in assignement", pos)),
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
        Expr::Integer(i) => Ok(ExprOutput { t: ExprType::Immediate(*i), zero: FlagsState::Unknown }),
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
                "X" => Ok(ExprOutput { t: ExprType::X, zero: FlagsState::Unknown }),
                "Y" => Ok(ExprOutput { t: ExprType::Y, zero: FlagsState::Unknown }),
                variable => match sub {
                    Subscript::None => Ok(ExprOutput { t: ExprType::Absolute(variable), zero: FlagsState::Unknown }),
                    Subscript::X => Ok(ExprOutput { t: ExprType::AbsoluteX(variable), zero: FlagsState::Unknown }),
                    Subscript::Y => Ok(ExprOutput { t: ExprType::AbsoluteY(variable), zero: FlagsState::Unknown }),
                }
            }
        },
        _ => unreachable!() 
    }
}

fn generate_for_loop(init: &Expr, condition: &Expr, update: &Expr, body: &StatementLoc, state: &State, gstate: &mut GeneratorState, pos: usize) -> Result<(), Error>
{
    generate_expr(init, state, gstate, pos)?;
    Err(Error::Unimplemented { feature: "for loop statement not implemented" })
}

fn generate_statement(code: &StatementLoc, state: &State, gstate: &mut GeneratorState) -> Result<(), Error>
{
    // Generate different kind of statements
    match &code.statement {
        Statement::Block(statements) => {
            for code in statements {
                generate_statement(&code, state, gstate)?;
            }
            Ok(())
        },
        Statement::Expression(expr) => { generate_expr(&expr, state, gstate, code.pos)?; Ok(()) },
        Statement::For { init, condition, update, body } => generate_for_loop(init, condition, update, body.as_ref(), state, gstate, code.pos),
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
        generate_statement(&f.1.code, state, &mut gstate)?;
        gstate.write_asm("RTS", 6)?;
    }

    Ok(())
}
