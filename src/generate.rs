use crate::error::Error;
use crate::compile::*;

use std::io::Write;

use log::debug;

// TODO: implement holdback on write (in order to remove PLA/PHA and LDA/STA pairs) 

struct GeneratorState<'a> {
    compiler_state: &'a CompilerState<'a>,
    last_included_line_number: usize,
    last_included_position: usize,
    last_included_char: std::str::Chars<'a>,
    writer: &'a mut dyn Write,
    local_label_counter_for: u32,
    local_label_counter_if: u32,
    local_label_counter_while: u32,
    loops: Vec<(String,String)>,
    flags: FlagsState<'a>,
    acc_in_use: bool,
    insert_code: bool,
    deferred_plusplus: Vec<(ExprType<'a>, usize, bool)>
}

impl<'a> GeneratorState<'a> {
    fn write(&mut self, s: &str) -> Result<usize, std::io::Error> {
        self.writer.write(s.as_bytes())
    }
    fn write_asm(&mut self, asm: &str, cycles: u32) -> Result<usize, std::io::Error> {
        if self.insert_code {
            self.writer.write(format!("\t{:23}\t; {} cycles\n", asm, cycles).as_bytes())
        } else {
            self.writer.write(format!("\t{}\n", asm).as_bytes())
        }
    }
    fn write_label(&mut self, label: &str) -> Result<usize, std::io::Error> {
        self.flags = FlagsState::Unknown; // There is a label, so there are some jumps to it -
                                          // flags are then unknown at that point
        self.write(label)?;
        self.write(&"\n")
    }
    fn purge_deferred_plusplus(&mut self) -> Result<(), Error> {
        let def = self.deferred_plusplus.clone();
        self.deferred_plusplus.clear();
        for d in def {
            generate_plusplus(&d.0, self, d.1, d.2)?;
        }
        Ok(())
    }
}

#[derive(Debug, Copy, Clone, PartialEq)]
enum ExprType<'a> {
    Nothing,
    Immediate(i32),
    Tmp(bool),
    Absolute(&'a str, u16),
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
    A(bool), X, Y,
}

#[derive(Debug, PartialEq, Copy, Clone)]
enum FlagsState<'a> {
    Unknown,
    X, Y,
    Zero, Positive, Negative,
    Absolute(&'a str, u16),
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
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
    let left = generate_expr(lhs, gstate, pos)?;
    match left {
        ExprType::X => {
            match generate_expr(rhs, gstate, pos)? {
                ExprType::Immediate(v) => {
                    gstate.write_asm(&format!("LDX #{}", v), 2)?;
                    gstate.flags = if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero };
                    Ok(ExprType::X) 
                },
                ExprType::Tmp(_) => {
                    gstate.write_asm("LDX cctmp", 3)?;
                    gstate.flags = FlagsState::X;
                    Ok(ExprType::X)
                },
                ExprType::Absolute(name, offset) => {
                    let v = gstate.compiler_state.get_variable(name);
                    if offset > 0 {
                        gstate.write_asm(&format!("LDX {}+{}", name, offset), if v.memory == VariableMemory::Zeropage {3} else {4})?;
                    } else {
                        gstate.write_asm(&format!("LDX {}", name), if v.memory == VariableMemory::Zeropage {3} else {4})?;
                    }
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
                ExprType::A(_) => {
                    gstate.write_asm(&"TAX", 2)?;
                    gstate.flags = FlagsState::X;
                    gstate.acc_in_use = false;
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
        ExprType::Y => {
            match generate_expr(rhs, gstate, pos)? {
                ExprType::Immediate(v) => {
                    gstate.write_asm(&format!("LDY #{}", v), 2)?;
                    gstate.flags = if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero };
                    Ok(ExprType::Y) 
                },
                ExprType::Tmp(_) => {
                    gstate.write_asm("LDY cctmp", 3)?;
                    gstate.flags = FlagsState::X;
                    Ok(ExprType::Y)
                },
                ExprType::Absolute(name, offset) => {
                    let v = gstate.compiler_state.get_variable(name);
                    if offset > 0 {
                        gstate.write_asm(&format!("LDY {}+{}", name, offset), if v.memory == VariableMemory::Zeropage {3} else {4})?;
                    } else {
                        gstate.write_asm(&format!("LDY {}", name), if v.memory == VariableMemory::Zeropage {3} else {4})?;
                    }
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
                ExprType::A(_)=> {
                    gstate.write_asm(&"TAY", 2)?;
                    gstate.acc_in_use = false;
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
        _ => {
            let right = generate_expr(rhs, gstate, pos)?; 
            match right {
                ExprType::X => {
                    match left {
                        ExprType::Absolute(variable, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if offset > 0 {
                                gstate.write_asm(&format!("STX {}+{}", variable, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("STX {}", variable), cycles)?;
                            }
                            Ok(ExprType::X)
                        },
                        ExprType::AbsoluteX(variable) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&"TXA", 2)?;
                            gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                            if gstate.acc_in_use {
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::X;
                            }
                            Ok(ExprType::X)
                        },
                        ExprType::AbsoluteY(variable) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            if v.memory == VariableMemory::Zeropage {
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
                            }
                            Ok(ExprType::X)
                        },
                        _ => Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
                    }
                },
                ExprType::Y => {
                    match left {
                        ExprType::Absolute(variable, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if offset > 0 {
                                gstate.write_asm(&format!("STY {}+{}", variable, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("STY {}", variable), cycles)?;
                            }
                            Ok(ExprType::Y)
                        },
                        ExprType::AbsoluteY(variable) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&"TYA", 2)?;
                            gstate.write_asm(&format!("STA {},Y", variable), cycles + 1)?;
                            if gstate.acc_in_use {
                                gstate.write_asm("PLA", 3)?;
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                gstate.flags = FlagsState::Y;
                            }
                            Ok(ExprType::Y)
                        },
                        ExprType::AbsoluteX(variable) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            if v.memory == VariableMemory::Zeropage {
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
                            }
                            Ok(ExprType::Y)
                        },
                        _ => Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
                    }
                },
                _ => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    match right {
                        ExprType::Absolute(variable, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if offset > 0 {
                                gstate.write_asm(&format!("LDA {}+{}", variable, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("LDA {}", variable), cycles)?;
                            }
                            gstate.flags = FlagsState::Absolute(variable, offset);
                        },
                        ExprType::AbsoluteX(variable) => {
                            gstate.write_asm(&format!("LDA {},X", variable), 4)?;
                            gstate.flags = FlagsState::AbsoluteX(variable);
                        },
                        ExprType::AbsoluteY(variable) => {
                            gstate.write_asm(&format!("LDA {},Y", variable), 4)?;
                            gstate.flags = FlagsState::AbsoluteY(variable);
                        },
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("LDA #{}", v), 2)?;
                            gstate.flags = if v > 0 { FlagsState::Positive } else if v < 0 { FlagsState::Negative } else { FlagsState::Zero };
                        },
                        ExprType::Tmp(_) => {
                            gstate.write_asm("LDA cctmp", 3)?;
                            gstate.flags = FlagsState::Unknown;
                        },
                        ExprType::A(_) => gstate.acc_in_use = false,
                        _ => unreachable!()
                    };
                    match left {
                        ExprType::Absolute(variable, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if offset > 0 {
                                gstate.write_asm(&format!("STA {}+{}", variable, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("STA {}", variable), cycles)?;
                            }
                        },
                        ExprType::AbsoluteX(variable) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            gstate.write_asm(&format!("STA {},X", variable), cycles + 1)?;
                        },
                        ExprType::AbsoluteY(variable) => {
                            gstate.write_asm(&format!("STA {},Y", variable), 5)?;
                        },
                        _ => return Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
                    };
                    if gstate.acc_in_use {
                        gstate.write_asm("PLA", 3)?;
                        gstate.flags = FlagsState::Unknown;
                    }
                    Ok(left)
                }
            }
        }
    }
}

fn generate_arithm<'a>(lhs: &Expr<'a>, op: &Operation, rhs: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    let acc_in_use = gstate.acc_in_use;
    if acc_in_use { gstate.write_asm("PHA", 3)?; }
    let left = generate_expr(lhs, gstate, pos)?;
    let right = generate_expr(rhs, gstate, pos)?;
    match left {
        ExprType::Immediate(l) => {
            match right {
                ExprType::Immediate(r) => {
                    match op {
                        Operation::Add => return Ok(ExprType::Immediate(l + r)),
                        Operation::Sub => return Ok(ExprType::Immediate(l - r)),
                        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
                    } 
                },
                _ => gstate.write_asm(&format!("LDA #{}", l), 2)?,
            };
        },
        ExprType::Absolute(varname, offset) => {
            let v = gstate.compiler_state.get_variable(varname);
            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
            if offset > 0 {
                gstate.write_asm(&format!("LDA {}+{}", varname, offset), cycles)?;
            } else {
                gstate.write_asm(&format!("LDA {}", varname), cycles)?;
            }
        },
        ExprType::AbsoluteX(varname) => {
            gstate.write_asm(&format!("LDA {},X", varname), 4)?;
        },
        ExprType::AbsoluteY(varname) => {
            gstate.write_asm(&format!("LDA {},Y", varname), 4)?;
        },
        ExprType::X => {
            gstate.write_asm("TXA", 2)?;
        },
        ExprType::Y => {
            gstate.write_asm("TYA", 2)?;
        },
        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
    }
    gstate.acc_in_use = true;
    let operation = match op {
        Operation::Add => {
            gstate.write_asm("CLC", 2)?;
            "ADC"
        },
        Operation::Sub => {
            gstate.write_asm("SEC", 2)?;
            "SBC"
        },
        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
    };
    let signed;
    match right {
        ExprType::Immediate(v) => {
            gstate.write_asm(&format!("{} #{}", operation, v), 2)?;
            signed = if v > 127 { false } else { true };
        },
        ExprType::Absolute(varname, offset) => {
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
            if offset > 0 {
                gstate.write_asm(&format!("{} {}+{}", operation, varname, offset), cycles)?;
            } else {
                gstate.write_asm(&format!("{} {}", operation, varname), cycles)?;
            }
        },
        ExprType::AbsoluteX(varname) => {
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            gstate.write_asm(&format!("{} {},X", operation, varname), 4)?;
        },
        ExprType::AbsoluteY(varname) => {
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            gstate.write_asm(&format!("{} {},Y", operation, varname), 4)?;
        },
        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
    };
    gstate.flags = FlagsState::Unknown;
    if acc_in_use {
        gstate.write_asm("STA cctmp", 3)?;
        gstate.write_asm("PLA", 3)?;
        Ok(ExprType::Tmp(signed))
    } else {
        Ok(ExprType::A(signed))
    }
}

fn generate_function_call<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    match expr {
        Expr::Identifier((var, sub)) => {
            match sub.as_ref() {
                Expr::Nothing => {
                    match gstate.compiler_state.functions.get(*var) {
                        None => Err(syntax_error(gstate.compiler_state, "Unknown function identifier", pos)),
                        Some(_) => {
                            let acc_in_use = gstate.acc_in_use;
                            if acc_in_use { gstate.write_asm("PHA", 3)?; }
                            gstate.write_asm(&format!("JSR {}", *var), 6)?;
                            if acc_in_use { gstate.write_asm("PLA", 3)?; }
                            gstate.flags = FlagsState::Unknown;
                            Ok(())
                        }
                    }
                },
                _ => Err(syntax_error(gstate.compiler_state, "No subscript allowed here", pos))
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Function call on something else than a function", pos))
    }
}

fn generate_plusplus<'a>(expr_type: &ExprType<'a>, gstate: &mut GeneratorState<'a>, pos: usize, plusplus: bool) -> Result<ExprType<'a>, Error>
{
    match expr_type {
        ExprType::X => {
            if plusplus {
                gstate.write_asm("INX", 2)?;
            } else {
                gstate.write_asm("DEX", 2)?;
            }
            gstate.flags = FlagsState::X;
            Ok(ExprType::X)
        },
        ExprType::Y => {
            if plusplus {
                gstate.write_asm("INY", 2)?;
            } else {
                gstate.write_asm("DEY", 2)?;
            }
            gstate.flags = FlagsState::Y;
            Ok(ExprType::Y)
        },
        ExprType::Absolute(variable, offset) => {
            let v = gstate.compiler_state.get_variable(variable);
            let cycles = if v.memory == VariableMemory::Zeropage { 5 } else { 6 };
            if *offset > 0 {
                if plusplus {
                    gstate.write_asm(&format!("INC {}+{}", variable, offset), cycles)?;
                } else {
                    gstate.write_asm(&format!("DEC {}+{}", variable, offset), cycles)?;
                }
            } else {
                if plusplus {
                    gstate.write_asm(&format!("INC {}", variable), cycles)?;
                } else {
                    gstate.write_asm(&format!("DEC {}", variable), cycles)?;
                }
            }
            gstate.flags = FlagsState::Absolute(variable, *offset);
            Ok(ExprType::Absolute(variable, *offset))
        },
        ExprType::AbsoluteX(variable) => {
            let v = gstate.compiler_state.get_variable(variable);
            let cycles = if v.memory == VariableMemory::Zeropage { 6 } else { 7 };
            if plusplus {
                gstate.write_asm(&format!("INC {}", variable), cycles)?;
            } else {
                gstate.write_asm(&format!("DEC {}", variable), cycles)?;
            }
            gstate.flags = FlagsState::AbsoluteX(variable);
            Ok(ExprType::AbsoluteX(variable))
        },
        _ => {
            if plusplus {
                Err(syntax_error(gstate.compiler_state, "Bad left value used with ++ operator", pos))
            } else {
                Err(syntax_error(gstate.compiler_state, "Bad left value used with -- operator", pos))
            }
        },
    }
}

fn generate_neg<'a>(expr: &Expr<'a>, _gstate: &mut GeneratorState<'a>, _pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(-*i)),
        _ => { return Err(Error::Unimplemented { feature: "negation statement is partially implemented" }); },
    }
}

fn generate_deref<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Identifier((var, sub)) => {
            let v = gstate.compiler_state.get_variable(var);
            if v.var_type == VariableType::CharPtr {
                let sub_output = generate_expr(sub, gstate, pos)?;
                match sub_output {
                    ExprType::Nothing => {
                        Ok(ExprType::Absolute(var, 0))
                    },
                    _ => Err(syntax_error(gstate.compiler_state, "No subscript is allowed in this context", pos))
                }
            } else {
                Err(syntax_error(gstate.compiler_state, "Deref on something else than a pointer", pos))
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Deref only works on pointers", pos)),
    }
}

fn generate_expr<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    debug!("Expression: {:?}", expr);
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(*i)),
        Expr::BinOp {lhs, op, rhs} => {
            match op {
                Operation::Assign => generate_assign(lhs, rhs, gstate, pos),
                Operation::Add | Operation::Sub => generate_arithm(lhs, op, rhs, gstate, pos),
                Operation::Eq => Err(Error::Unimplemented { feature: "Equal not implemented" }),
                Operation::Neq => Err(Error::Unimplemented { feature: "Not equal not implemented" }),
                Operation::Gt => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
                Operation::Gte => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
                Operation::Lt => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
                Operation::Lte => Err(Error::Unimplemented { feature: "Comparison not implemented" }),
            }
        },
        Expr::Identifier((var, sub)) => {
            match *var {
                "X" => Ok(ExprType::X),
                "Y" => Ok(ExprType::Y),
                variable => {
                    let v = gstate.compiler_state.get_variable(variable);
                    if let VariableDefinition::Value(val) = &v.def {
                        Ok(ExprType::Immediate(*val))
                    } else {
                        let sub_output = generate_expr(sub, gstate, pos)?;
                        match sub_output {
                            ExprType::Nothing => Ok(ExprType::Absolute(variable, 0)),
                            ExprType::X => Ok(ExprType::AbsoluteX(variable)),
                            ExprType::Y => Ok(ExprType::AbsoluteY(variable)),
                            ExprType::Immediate(v) => Ok(ExprType::Absolute(variable, v as u16)),
                            _ => Err(syntax_error(gstate.compiler_state, "Subscript not allowed (only X, Y and constants are allowed)", pos))
                        }
                    }
                },
            }
        },
        Expr::FunctionCall(expr) => {
            generate_function_call(expr, gstate, pos)?;
            Ok(ExprType::Nothing)
        },
        Expr::MinusMinus(expr, false) => {
            let expr_type = generate_expr(expr, gstate, pos)?;
            generate_plusplus(&expr_type, gstate, pos, false)?;
            Ok(expr_type)
        },
        Expr::PlusPlus(expr, false) => {
            let expr_type = generate_expr(expr, gstate, pos)?;
            generate_plusplus(&expr_type, gstate, pos, true)?;
            Ok(expr_type)
        },
        Expr::MinusMinus(expr, true) => {
            let expr_type = generate_expr(expr, gstate, pos)?;
            gstate.deferred_plusplus.push((expr_type.clone(), pos, false));
            Ok(expr_type)
        },
        Expr::PlusPlus(expr, true) => {
            let expr_type = generate_expr(expr, gstate, pos)?;
            gstate.deferred_plusplus.push((expr_type.clone(), pos, true));
            Ok(expr_type)
        },
        Expr::Neg(v) => generate_neg(v, gstate, pos),
        Expr::Deref(v) => generate_deref(v, gstate, pos),
        Expr::Nothing => Ok(ExprType::Nothing),
    }
}

fn flags_ok(flags: &FlagsState, expr_type: &ExprType) -> bool
{
    match flags {
        FlagsState::X => *expr_type == ExprType::X,
        FlagsState::Y => *expr_type == ExprType::Y,
        FlagsState::Absolute(var, offset) => *expr_type == ExprType::Absolute(*var, *offset),
        FlagsState::AbsoluteX(var) => *expr_type == ExprType::AbsoluteX(var),
        FlagsState::AbsoluteY(var) => *expr_type == ExprType::AbsoluteY(var),
        _ => false
    }
}

fn generate_condition<'a>(condition: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize, negate: bool, label: &str) -> Result<(), Error>
{
    debug!("Condition: {:?}", condition);
    match condition {
        Expr::BinOp {lhs, op, rhs} => {
            let l = generate_expr(lhs, gstate, pos)?;
            let r = generate_expr(rhs, gstate, pos)?;
            let left;
            let right;

            let switch = match &l {
                ExprType::X | ExprType::Y => {
                    left = &l; right = &r;
                    false
                }, 
                _ => match &r {
                    ExprType::A(_) => {
                        left = &r; right = &l;
                        true 
                    },
                    _ => {
                        left = &l; right = &r;
                        false
                    }
                }
            };
           
            let opx = if negate {
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
          
            let operator = if switch {
                match op {
                    Operation::Eq => Operation::Eq,
                    Operation::Neq => Operation::Neq,
                    Operation::Gt => Operation::Lt,
                    Operation::Gte => Operation::Lte,
                    Operation::Lt => Operation::Gt,
                    Operation::Lte => Operation::Gte,
                    _ => unreachable!()
                }
            } else { opx };

            if let ExprType::Immediate(v) = *right {
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
                ExprType::Absolute(varname, offset) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    let v = gstate.compiler_state.get_variable(varname);
                    signed = v.signed;
                    let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                    if *offset > 0 {
                        gstate.write_asm(&format!("LDA {}+{}", varname, offset), cycles)?;
                    } else {
                        gstate.write_asm(&format!("LDA {}", varname), cycles)?;
                    }
                    cmp = true;
                },
                ExprType::AbsoluteX(varname) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    let v = gstate.compiler_state.get_variable(varname);
                    signed = v.signed;
                    gstate.write_asm(&format!("LDA {},X", varname), 4)?;
                    cmp = true;
                },
                ExprType::AbsoluteY(varname) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    let v = gstate.compiler_state.get_variable(varname);
                    signed = v.signed;
                    gstate.write_asm(&format!("LDA {},Y", varname), 4)?;
                    cmp = true;
                },
                ExprType::A(sign) => {
                    cmp = true;
                    signed = *sign;
                    gstate.acc_in_use = false;
                },
                ExprType::Tmp(sign) => {
                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                    signed = *sign;
                    gstate.write_asm("LDA cctmp", 3)?;
                    cmp = true;
                },
                ExprType::Y => {
                    signed = false;
                    match right {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("CPY #{}", v), 2)?;
                            cmp = false;
                        },
                        ExprType::Absolute(varname, offset) => {
                            let v = gstate.compiler_state.get_variable(varname);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if *offset > 0 {
                                gstate.write_asm(&format!("CPY {}+{}", varname, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("CPY {}", varname), cycles)?;
                            }
                            cmp = false;
                        },
                        ExprType::A(_) => {
                            gstate.write_asm("STA cctmp", 3)?;
                            gstate.write_asm("CPY cctmp", 3)?;
                            cmp = false;
                            gstate.acc_in_use = false;
                        }
                        _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
                    } 
                },
                ExprType::X => {
                    signed = false;
                    match right {
                        ExprType::Immediate(v) => {
                            gstate.write_asm(&format!("CPX #{}", v), 2)?;
                            cmp = false;
                        },
                        ExprType::Absolute(varname, offset) => {
                            let v = gstate.compiler_state.get_variable(varname);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if *offset > 0 {
                                gstate.write_asm(&format!("CPX {}+{}", varname, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("CPX {}", varname), cycles)?;
                            }
                            cmp = false;
                        },
                        ExprType::A(_) => {
                            gstate.write_asm(&format!("STA cctmp"), 3)?;
                            gstate.write_asm(&format!("CPX cctmp"), 3)?;
                            cmp = false;
                            gstate.acc_in_use = false;
                        }
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
                    ExprType::Absolute(name, offset) => {
                        let v = gstate.compiler_state.get_variable(name);
                        let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                        if *offset > 0 {
                            gstate.write_asm(&format!("CMP {}+{}", name, offset), cycles)?;
                        } else {
                            gstate.write_asm(&format!("CMP {}", name), cycles)?;
                        }
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
        _ => {
            let cmp;
            let expr = generate_expr(condition, gstate, pos)?;
            if flags_ok(&gstate.flags, &expr) {
                if negate {
                    gstate.write_asm(&format!("BEQ {}", label), 2)?;
                } else {
                    gstate.write_asm(&format!("BNE {}", label), 2)?;
                }
                Ok(())
            } else {
                gstate.flags = FlagsState::Unknown;
                match expr {
                    ExprType::Immediate(v) => {
                        if v != 0 {
                            if !negate {
                                gstate.write_asm(&format!("JMP {}", label), 3)?;
                            }
                        } else {
                            if negate {
                                gstate.write_asm(&format!("JMP {}", label), 3)?;
                            }
                        }
                        cmp = false;
                    },
                    ExprType::Absolute(varname, offset) => {
                        if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                        let v = gstate.compiler_state.get_variable(varname);
                        let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                        if offset > 0 {
                            gstate.write_asm(&format!("LDA {}+{}", varname, offset), cycles)?;
                        } else {
                            gstate.write_asm(&format!("LDA {}", varname), cycles)?;
                        }
                        cmp = true;
                    },
                    ExprType::AbsoluteX(varname) => {
                        if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                        gstate.write_asm(&format!("LDA {},X", varname), 4)?;
                        cmp = true;
                    },
                    ExprType::AbsoluteY(varname) => {
                        if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                        gstate.write_asm(&format!("LDA {},Y", varname), 4)?;
                        cmp = true;
                    },
                    ExprType::A(_) => {
                        cmp = true;
                        gstate.acc_in_use = false;
                    },
                    ExprType::Y => {
                        gstate.write_asm("CPY #0", 2)?;
                        cmp = false;
                    },
                    ExprType::X => {
                        gstate.write_asm("CPX #0", 2)?;
                        cmp = false;
                    }
                    ExprType::Tmp(_) => {
                        if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                        gstate.write_asm("LDA cctmp", 3)?;
                        cmp = true;
                    },
                    _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
                }

                if cmp {
                    gstate.write_asm("CMP #0", 2)?;
                    if gstate.acc_in_use { 
                        gstate.write_asm("PLA", 3)?; 
                    }
                }
                if negate {
                    gstate.write_asm(&format!("BEQ {}", label), 2)?;
                } else {
                    gstate.write_asm(&format!("BNE {}", label), 2)?;
                }
                Ok(())
            }
        }
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
    gstate.purge_deferred_plusplus()?;
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

fn generate_while<'a>(condition: &Expr<'a>, body: &StatementLoc<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    gstate.local_label_counter_while += 1;
    let while_label = format!(".while{}", gstate.local_label_counter_while);
    let whileend_label = format!(".whileend{}", gstate.local_label_counter_while);
    gstate.loops.push((while_label.clone(), whileend_label.clone()));
    gstate.write_label(&while_label)?;
    generate_condition(condition, gstate, pos, true, &whileend_label)?;
    generate_statement(body, gstate)?;
    gstate.write_asm(&format!("JMP {}", while_label), 3)?;
    gstate.write_label(&whileend_label)?;
    gstate.loops.pop();
    Ok(())
}

fn generate_do_while<'a>(body: &StatementLoc<'a>, condition: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    gstate.local_label_counter_while += 1;
    let dowhile_label = format!(".dowhile{}", gstate.local_label_counter_while);
    let dowhilecondition_label = format!(".dowhilecondition{}", gstate.local_label_counter_while);
    let dowhileend_label = format!(".dowhileend{}", gstate.local_label_counter_while);
    gstate.loops.push((dowhilecondition_label.clone(), dowhileend_label.clone()));
    gstate.write_label(&dowhile_label)?;
    generate_statement(body, gstate)?;
    gstate.write_label(&dowhilecondition_label)?;
    generate_condition(condition, gstate, pos, false, &dowhile_label)?;
    gstate.write_label(&dowhileend_label)?;
    gstate.loops.pop();
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

fn generate_asm_statement<'a>(s: &'a str, gstate: &mut GeneratorState<'a>) -> Result<(), Error>
{
    gstate.write(&format!("\t{s}\n"))?;
    Ok(())
}

fn generate_strobe_statement<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    match expr {
        Expr::Identifier((name, _)) => {
            let v = gstate.compiler_state.get_variable(name);
            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
            match v.var_type {
                VariableType::CharPtr => {
                    gstate.write_asm(&format!("STA {}", name), cycles)?;
                    Ok(())
                },
                _ => Err(syntax_error(gstate.compiler_state, "Strobe only works on memory pointers", pos)),
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Strobe only works on memory pointers", pos)),
    }
}

fn generate_statement<'a>(code: &StatementLoc<'a>, gstate: &mut GeneratorState<'a>) -> Result<(), Error>
{
    // Include C source code into generated asm
    // debug!("{:?}, {}, {}, {}", expr, pos, gstate.last_included_position, gstate.last_included_line_number);
    if gstate.insert_code {
        let included_source_code = generate_included_source_code_line(code.pos, gstate);
        let line_to_be_written = if let Some(line) = included_source_code {
            Some(line.to_string())
        } else {
            None
        };
        // debug!("{:?}, {}, {}", line_to_be_written, gstate.last_included_position, gstate.last_included_line_number);
        if let Some(l) = line_to_be_written {
            gstate.write(";")?;
            gstate.write(&l)?; // Should include the '\n'
        }
    }

    gstate.purge_deferred_plusplus()?;
    
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
        Statement::While { condition, body } => { 
            generate_while(condition, body.as_ref(), gstate, code.pos)?; 
        },
        Statement::DoWhile { body, condition } => { 
            generate_do_while(body.as_ref(), condition, gstate, code.pos)?; 
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
        Statement::Asm(s) => { generate_asm_statement(s, gstate)?; }
        Statement::Strobe(s) => { generate_strobe_statement(s, gstate, code.pos)?; }
    }
    
    gstate.purge_deferred_plusplus()?;
    Ok(())
}

pub fn generate_asm(compiler_state: &CompilerState, writer: &mut dyn Write, insert_code: bool) -> Result<(), Error> 
{
    let mut gstate = GeneratorState {
        compiler_state,
        last_included_line_number: 0,
        last_included_position: 0,
        last_included_char: compiler_state.preprocessed_utf8.chars(),
        writer,
        local_label_counter_for: 0,
        local_label_counter_if: 0,
        local_label_counter_while: 0,
        loops: Vec::new(),
        flags: FlagsState::Unknown,
        acc_in_use: false,
        insert_code,
        deferred_plusplus: Vec::new()
    };

    gstate.write("\tPROCESSOR 6502\n\n")?;
    
    for v in compiler_state.sorted_variables().iter() {
        if v.1.var_const  {
            if let VariableDefinition::Value(val) = &v.1.def  {
                gstate.write(&format!("{:23}\tEQU ${:x}\n", v.0, val))?;
            }
        }
    }

    gstate.write("\n\tSEG.U VARS\n\tORG $80\n\n")?;
    
    // Generate variables code
    gstate.write("cctmp                  \tds 1\n")?; 
    for v in compiler_state.sorted_variables().iter() {
        if !v.1.var_const && v.1.memory == VariableMemory::Zeropage && v.1.def == VariableDefinition::None {
            let s = match v.1.var_type {
                VariableType::Char => 1,
                VariableType::Short => 2,
                VariableType::CharPtr => 2,
            };
            gstate.write(&format!("{:23}\tds {}\n", v.0, v.1.size * s))?; 
        }
    }

    gstate.write("\n\tSEG CODE\n\tORG $F000\n")?;

    // Generate functions code
    gstate.write("\n; Functions definitions\n")?;
    for f in compiler_state.sorted_functions().iter() {
        gstate.write(&format!("\n{}\tSUBROUTINE\n", f.0))?;
        gstate.local_label_counter_for = 0;
        gstate.local_label_counter_if = 0;
        generate_statement(&f.1.code, &mut gstate)?;
        gstate.write_asm("RTS", 6)?;
    }
   
    // Generate startup code
    gstate.write("
Powerup
        SEI		; Set the interrupt masking flag in the processor status register.
      ; The VCS has no interrupts, but the Atari 7800 which can run VCS
      ; software does have interrupts so we need to turn them off to be
      ; compatible with the 7800.

        CLD		; Clear the BCD mode flag in the processor status register.  At
      ; powerup the processor status register flags can be in any state.
      ; We do not want to perform math in BCD mode at this time, so we
      ; make sure it is turned off.

        LDX #$FF	; We set the stack pointer (SP) register in the processor to point
        TXS		; at the highest address of RAM.  If we use the stack then it will
      ; consume memory space growing downward, starting at that top address.

        LDA #0		; This loop clears every byte of RAM in the VCS, and every writable
.loop	  STA $00,X	; register in the TIA chip to the known state of zero.   Remember at
        DEX		; power up the bits of RAM and Registers are in an unknown state.
        BNE .loop	; If we neglected to put them into a known state, then our program 
      ; may behave differently each time it runs.  We chose to set them all
      ; to zero, because in the TIA setting a register to zero tends to
      ; turn features off.  We will later turn back on only the features
      ; of the TIA we wish to use in our program.

        JMP main

        ECHO ([$FFFC-.]d), \"bytes free\"

        ORG $FFFA

        .word Powerup\t; NMI
        .word Powerup\t; RESET
        .word Powerup\t; IRQ

        END\n")?;

    Ok(())
}
