use crate::error::Error;
use crate::compile::*;
use crate::assemble::Assembly;

use std::io::Write;

use log::debug;

// TODO: add cctmp in use

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
    deferred_plusplus: Vec<(ExprType<'a>, usize, bool)>,
    bankswitching_method: &'static str,
    current_bank: u32,
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
pub enum ExprType<'a> {
    Nothing,
    Immediate(i32),
    Tmp(bool),
    Absolute(&'a str, bool, u16), // variable, eight_bits, offset
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
    A(bool), X, Y,
    Label(&'a str),
}

#[derive(Debug, PartialEq, Copy, Clone)]
enum FlagsState<'a> {
    Unknown,
    X, Y,
    Zero, Positive, Negative,
    Absolute(&'a str, bool, u16),
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

fn generate_assign<'a>(left: &ExprType<'a>, right: &ExprType<'a>, gstate: &mut GeneratorState<'a>, pos: usize, high_byte: bool) -> Result<ExprType<'a>, Error>
{
    match left {
        ExprType::X => {
            match right {
                ExprType::Immediate(v) => {
                    gstate.write_asm(&format!("LDX #{}", v), 2)?;
                    gstate.flags = FlagsState::X; 
                    Ok(ExprType::X) 
                },
                ExprType::Tmp(_) => {
                    gstate.write_asm("LDX cctmp", 3)?;
                    gstate.flags = FlagsState::X;
                    Ok(ExprType::X)
                },
                ExprType::Absolute(name, eight_bits, offset) => {
                    let v = gstate.compiler_state.get_variable(name);
                    if !eight_bits {
                        return Err(syntax_error(gstate.compiler_state, "Can't assign 16 bits data to X", pos));
                    }
                    if *offset > 0 {
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
                    // TODO; Check if the target variable is a pointer (indirect adressing)
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
                ExprType::Nothing => unreachable!(),
                ExprType::Label(_) => unreachable!(),
            }
        },
        ExprType::Y => {
            match right {
                ExprType::Immediate(v) => {
                    gstate.write_asm(&format!("LDY #{}", v), 2)?;
                    gstate.flags = FlagsState::Y; 
                    Ok(ExprType::Y) 
                },
                ExprType::Tmp(_) => {
                    gstate.write_asm("LDY cctmp", 3)?;
                    gstate.flags = FlagsState::Y;
                    Ok(ExprType::Y)
                },
                ExprType::Absolute(name, eight_bits, offset) => {
                    let v = gstate.compiler_state.get_variable(name);
                    if !eight_bits {
                        return Err(syntax_error(gstate.compiler_state, "Can't assign 16 bits data to Y", pos));
                    }
                    if *offset > 0 {
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
                ExprType::Nothing => unreachable!(),
                ExprType::Label(_) => unreachable!(),
            }
        },
        _ => {
            match right {
                ExprType::X => {
                    match left {
                        ExprType::Absolute(variable, eight_bits, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if *offset > 0 {
                                gstate.write_asm(&format!("STX {}+{}", variable, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("STX {}", variable), cycles)?;
                            }
                            if !eight_bits {
                                if *offset == 0 {
                                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                                    gstate.write_asm("LDA #0", 2)?;
                                    gstate.write_asm(&format!("STA {}+1", variable), cycles)?;
                                    if gstate.acc_in_use { 
                                        gstate.write_asm("PLA", 3)?;
                                        gstate.flags = FlagsState::Unknown;
                                    } else {
                                        gstate.flags = FlagsState::Zero;
                                    }
                                } else {
                                    unreachable!(); 
                                }
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
                        ExprType::A(_) => {
                            if gstate.acc_in_use {
                                return Err(syntax_error(gstate.compiler_state, "Code too complex for the compiler", pos))
                            }
                            gstate.write_asm(&"TXA", 2)?;
                            gstate.acc_in_use = true;
                            return Ok(ExprType::A(false));
                        },
                        _ => Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
                    }
                },
                ExprType::Y => {
                    match left {
                        ExprType::Absolute(variable, eight_bits, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            if *offset > 0 {
                                gstate.write_asm(&format!("STY {}+{}", variable, offset), cycles)?;
                            } else {
                                gstate.write_asm(&format!("STY {}", variable), cycles)?;
                            }
                            if !eight_bits {
                                if *offset == 0 {
                                    if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                                    gstate.write_asm("LDA #0", 2)?;
                                    gstate.write_asm(&format!("STA {}+1", variable), cycles)?;
                                    if gstate.acc_in_use { 
                                        gstate.write_asm("PLA", 3)?;
                                        gstate.flags = FlagsState::Unknown;
                                    } else {
                                        gstate.flags = FlagsState::Zero;
                                    }
                                } else {
                                    unreachable!(); 
                                }
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
                        ExprType::A(_) => {
                            if gstate.acc_in_use {
                                return Err(syntax_error(gstate.compiler_state, "Code too complex for the compiler", pos))
                            }
                            gstate.write_asm(&"TYA", 2)?;
                            gstate.acc_in_use = true;
                            return Ok(ExprType::A(false));
                        },
                        _ => Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
                    }
                },
                _ => {
                    let mut acc_in_use = gstate.acc_in_use;
                    let signed;
                    match right {
                        ExprType::Absolute(variable, eight_bits, offset) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            let v = gstate.compiler_state.get_variable(variable);
                            signed = v.signed;
                            if v.var_type == VariableType::CharPtr && !eight_bits && v.var_const {
                                if high_byte {
                                    gstate.write_asm(&format!("LDA #>{}", variable), 2)?;
                                } else {
                                    gstate.write_asm(&format!("LDA #<{}", variable), 2)?;
                                }
                                gstate.flags = FlagsState::Unknown;
                            } else {
                                let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                                if high_byte && *eight_bits {
                                    gstate.write_asm("LDA #0", 2)?;
                                    gstate.flags = FlagsState::Zero;
                                } else {
                                    let off = if high_byte { *offset + 1 } else { *offset };
                                    if off > 0 {
                                        gstate.write_asm(&format!("LDA {}+{}", variable, off), cycles)?;
                                    } else {
                                        gstate.write_asm(&format!("LDA {}", variable), cycles)?;
                                    }
                                    gstate.flags = FlagsState::Absolute(variable, *eight_bits, off);
                                }
                            }
                        },
                        ExprType::AbsoluteX(variable) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            let v = gstate.compiler_state.get_variable(variable);
                            signed = v.signed;
                            gstate.write_asm(&format!("LDA {},X", variable), 4)?;
                            gstate.flags = FlagsState::AbsoluteX(variable);
                        },
                        ExprType::AbsoluteY(variable) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            let v = gstate.compiler_state.get_variable(variable);
                            signed = v.signed;
                            if v.var_type == VariableType::CharPtr && !v.var_const {
                                if v.size == 1 {
                                    gstate.write_asm(&format!("LDA ({}),Y", variable), 5)?;
                                } else {
                                    return Err(syntax_error(gstate.compiler_state, "X-Indirect adressing mode not available with Y register", pos));
                                }
                            } else {
                                gstate.write_asm(&format!("LDA {},Y", variable), 4)?;
                            }
                            gstate.flags = FlagsState::AbsoluteY(variable);
                        },
                        ExprType::Immediate(v) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            let vx = match high_byte {
                                false => v & 0xff,
                                true => (v >> 8) & 0xff,
                            };
                            signed = false;
                            gstate.write_asm(&format!("LDA #{}", vx), 2)?;
                            gstate.flags = if vx > 0 { FlagsState::Positive } else if vx < 0 { FlagsState::Negative } else { FlagsState::Zero };
                        },
                        ExprType::Tmp(s) => {
                            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                            signed = *s;
                            gstate.write_asm("LDA cctmp", 3)?;
                            gstate.flags = FlagsState::Unknown;
                        },
                        ExprType::A(s) => {
                            signed = *s;
                            acc_in_use = false;
                            gstate.acc_in_use = false;
                        },
                        _ => unreachable!()
                    };
                    match left {
                        ExprType::Absolute(variable, _eight_bits, offset) => {
                            let v = gstate.compiler_state.get_variable(variable);
                            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
                            let off = if high_byte { *offset + 1 } else { *offset };
                            if off > 0 {
                                gstate.write_asm(&format!("STA {}+{}", variable, off), cycles)?;
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
                        ExprType::A(_) => {
                            if acc_in_use {
                                return Err(syntax_error(gstate.compiler_state, "Code too complex for the compiler", pos))
                            }
                            gstate.acc_in_use = true;
                            return Ok(ExprType::A(signed));
                        },
                        ExprType::Tmp(_) => {
                            return Ok(ExprType::Tmp(signed));
                        },
                        _ => return Err(syntax_error(gstate.compiler_state, "Bad left value in assignement", pos)),
                    };
                    if acc_in_use {
                        gstate.write_asm("PLA", 3)?;
                        gstate.flags = FlagsState::Unknown;
                    }
                    Ok(*left)
                }
            }
        }
    }
}

fn generate_arithm<'a>(l: &ExprType<'a>, op: &Operation, r: &ExprType<'a>, gstate: &mut GeneratorState<'a>, pos: usize, high_byte: bool) -> Result<ExprType<'a>, Error>
{
    let mut acc_in_use = gstate.acc_in_use;
    debug!("Arithm: {:?},{:?},{:?}", l, op, r);    
    let left;
    let right;
    let right2;

    match op {
        Operation::Sub(_) | Operation::Div(_) => {
            left = l; right = r;
        },
        _ => {
            match r {
                ExprType::A(_) => {
                    left = r; right = l;
                },
                _ => {
                    left = l; right = r;
                }
            }
        }
    }

    let x;
    right2 = match right {
        ExprType::A(s) => {
            gstate.write_asm("STA cctmp", 3)?;
            acc_in_use = false;
            x = ExprType::Tmp(*s);
            &x
        },
        _ => {
            right
        }
    };

    match left {
        ExprType::Immediate(l) => {
            match right {
                ExprType::Immediate(r) => {
                    match op {
                        Operation::Add(_) => return Ok(ExprType::Immediate(l + r)),
                        Operation::Sub(_) => return Ok(ExprType::Immediate(l - r)),
                        Operation::And(_) => return Ok(ExprType::Immediate(l & r)),
                        Operation::Or(_) => return Ok(ExprType::Immediate(l | r)),
                        Operation::Xor(_) => return Ok(ExprType::Immediate(l ^ r)),
                        Operation::Mul(_) => return Ok(ExprType::Immediate(l * r)),
                        Operation::Div(_) => return Ok(ExprType::Immediate(l / r)),
                        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
                    } 
                },
                _ => {
                    if acc_in_use { gstate.write_asm("PHA", 3)?; }
                    if high_byte {
                        gstate.write_asm(&format!("LDA #{}", (l >> 8) & 0xff), 2)?
                    } else {
                        gstate.write_asm(&format!("LDA #{}", l & 0xff), 2)?
                    }
                }
            };
        },
        ExprType::Absolute(varname, eight_bits, offset) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            let v = gstate.compiler_state.get_variable(varname);
            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
            if high_byte && *eight_bits {
                gstate.write_asm("LDA #0", 2)?;
            } else {
                let off = if high_byte { *offset + 1 } else { *offset };
                if off > 0 {
                    gstate.write_asm(&format!("LDA {}+{}", varname, off), cycles)?;
                } else {
                    gstate.write_asm(&format!("LDA {}", varname), cycles)?;
                }
            }
        },
        ExprType::AbsoluteX(varname) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            gstate.write_asm(&format!("LDA {},X", varname), 4)?;
        },
        ExprType::AbsoluteY(variable) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            let v = gstate.compiler_state.get_variable(variable);
            if v.var_type == VariableType::CharPtr && !v.var_const {
                if v.size == 1 {
                    gstate.write_asm(&format!("LDA ({}),Y", variable), 5)?;
                } else {
                    return Err(syntax_error(gstate.compiler_state, "X-Indirect adressing mode not available with Y register", pos));
                }
            } else {
                gstate.write_asm(&format!("LDA {},Y", variable), 4)?;
            }
        },
        ExprType::X => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            gstate.write_asm("TXA", 2)?;
        },
        ExprType::Y => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            gstate.write_asm("TYA", 2)?;
        },
        ExprType::A(_) => {
            acc_in_use = false;
        },
        ExprType::Tmp(_) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            gstate.write_asm("LDA cctmp", 3)?;
        },
        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
    }
    gstate.acc_in_use = true;
    let operation = match op {
        Operation::Add(_) => {
            if !high_byte {
                gstate.write_asm("CLC", 2)?;
            }
            "ADC"
        },
        Operation::Sub(_) => {
            if !high_byte {
                gstate.write_asm("SEC", 2)?;
            }
            "SBC"
        },
        Operation::And(_) => {
            "AND"
        },
        Operation::Or(_) => {
            "ORA"
        },
        Operation::Xor(_) => {
            "EOR"
        },
        Operation::Mul(_) => { return Err(syntax_error(gstate.compiler_state, "Operation not possible. 6502 doesn't implement a multiplier.", pos)) },
        Operation::Div(_) => { return Err(syntax_error(gstate.compiler_state, "Operation not possible. 6502 doesn't implement a divider.", pos)) },
        _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
    };
    let signed;
    match right2 {
        ExprType::Immediate(v) => {
            let vx = match high_byte {
                false => v & 0xff,
                true => (v >> 8) & 0xff,
            };
            gstate.write_asm(&format!("{} #{}", operation, vx), 2)?;
            signed = if *v < 0 { true } else { false };
        },
        ExprType::Absolute(varname, eight_bits, offset) => {
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
            if high_byte && *eight_bits {
                gstate.write_asm(&format!("{} #0", operation), 2)?;
            } else {
                let off = if high_byte { *offset + 1 } else { *offset };
                if off > 0 {
                    gstate.write_asm(&format!("{} {}+{}", operation, varname, off), cycles)?;
                } else {
                    gstate.write_asm(&format!("{} {}", operation, varname), cycles)?;
                }
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
            if v.var_type == VariableType::CharPtr && !v.var_const {
                if v.size == 1 {
                    gstate.write_asm(&format!("{} ({}),Y", operation, varname), 5)?;
                } else {
                    return Err(syntax_error(gstate.compiler_state, "X-Indirect adressing mode not available with Y register", pos));
                }
            } else {
                gstate.write_asm(&format!("{} {},Y", operation, varname), 4)?;
            }
        },
        ExprType::X => {
            signed = false;
            gstate.write_asm("STX cctmp", 3)?;
            gstate.write_asm(&format!("{} cctmp", operation), 4)?;
        },
        ExprType::Y => {
            signed = false;
            gstate.write_asm("STY cctmp", 3)?;
            gstate.write_asm(&format!("{} cctmp", operation), 4)?;
        },
        ExprType::Tmp(s) => {
            signed = *s;
            gstate.write_asm(&format!("{} cctmp", operation), 4)?;
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

fn generate_shift<'a>(left: &ExprType<'a>, op: &Operation, right: &ExprType<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    let mut acc_in_use = gstate.acc_in_use;
    let signed;
    match left {
        ExprType::Absolute(varname, _eight_bits, offset) => {
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            let cycles = if v.memory == VariableMemory::Zeropage { 3 } else { 4 };
            if (v.var_type == VariableType::Short || v.var_type == VariableType::CharPtr) && *op == Operation::Brs(false) {
                // Special shift 8 case for extracting higher byte
                match right {
                    ExprType::Immediate(v) => {
                        if *v == 8 {
                            return Ok(ExprType::Absolute(varname, true, offset + 1));
                        } else {
                            return Err(syntax_error(gstate.compiler_state, "Incorrect right value for right shift operation on short (constant 8 only supported)", pos));
                        } 
                    },
                    _ => return Err(syntax_error(gstate.compiler_state, "Incorrect right value for right shift operation on short (constant 8 only supported)", pos))
                };
            } else {
                if acc_in_use { gstate.write_asm("PHA", 3)?; }
                if *offset > 0 {
                    gstate.write_asm(&format!("LDA {}+{}", varname, offset), cycles)?;
                } else {
                    gstate.write_asm(&format!("LDA {}", varname), cycles)?;
                }
            }
        },
        ExprType::AbsoluteX(varname) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            gstate.write_asm(&format!("LDA {},X", varname), 4)?;
        },
        ExprType::AbsoluteY(varname) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            let v = gstate.compiler_state.get_variable(varname);
            signed = v.signed; 
            gstate.write_asm(&format!("LDA {},Y", varname), 4)?;
        },
        ExprType::X => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            signed = false;
            gstate.write_asm("TXA", 2)?;
        },
        ExprType::Y => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            signed = false;
            gstate.write_asm("TYA", 2)?;
        },
        ExprType::A(s) => { 
            acc_in_use = false;
            signed = *s; 
        },
        ExprType::Tmp(s) => {
            if acc_in_use { gstate.write_asm("PHA", 3)?; }
            signed = *s;
            gstate.write_asm("LDA cctmp", 3)?;
        },
        _ => return Err(syntax_error(gstate.compiler_state, "Bad left value for shift operation", pos))
    }
    gstate.acc_in_use = true;
    let operation = match op {
        Operation::Brs(_) => {
            "LSR"
        },
        Operation::Bls(_) => {
            "ASL"
        },
        _ => unreachable!(),
    };
    match right {
        ExprType::Immediate(v) => {
            if *v >= 0 && *v <= 8 {
                for _ in 0..*v {
                    gstate.write_asm(&format!("{}", operation), 2)?;
                }
            } else {
                return Err(syntax_error(gstate.compiler_state, "Negative shift operation not allowed", pos));
            } 
        },
        _ => return Err(syntax_error(gstate.compiler_state, "Incorrect right value for shift operation (positive constants only)", pos))
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

fn generate_ternary<'a>(condition: &Expr<'a>, alternatives: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match alternatives {
        Expr::BinOp {lhs, op, rhs} => {
            if *op == Operation::TernaryCond2 {
                if gstate.acc_in_use {
                    gstate.write_asm("PHA", 3)?; 
                    gstate.local_label_counter_if += 1;
                    let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
                    let else_label = format!(".else{}", gstate.local_label_counter_if);
                    generate_condition(condition, gstate, pos, true, &else_label)?;
                    let left = generate_expr(lhs, gstate, pos, false)?;
                    let la = generate_assign(&ExprType::A(false), &left, gstate, pos, false)?;
                    gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
                    gstate.write_label(&else_label)?;
                    gstate.acc_in_use = false;
                    let right = generate_expr(rhs, gstate, pos, false)?;
                    let ra = generate_assign(&ExprType::A(false), &right, gstate, pos, false)?;
                    gstate.write_label(&ifend_label)?;
                    gstate.write_asm("STA cctmp", 3)?;
                    gstate.write_asm("PLA", 3)?;
                    if la != ra {
                        return Err(syntax_error(gstate.compiler_state, "Different alternative types in ?: expression", pos))
                    }
                    Ok(la)
                } else {
                    gstate.local_label_counter_if += 1;
                    let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
                    let else_label = format!(".else{}", gstate.local_label_counter_if);
                    generate_condition(condition, gstate, pos, true, &else_label)?;
                    let left = generate_expr(lhs, gstate, pos, false)?;
                    let la = generate_assign(&ExprType::A(false), &left, gstate, pos, false)?;
                    gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
                    gstate.write_label(&else_label)?;
                    gstate.acc_in_use = false;
                    let right = generate_expr(rhs, gstate, pos, false)?;
                    let ra = generate_assign(&ExprType::A(false), &right, gstate, pos, false)?;
                    gstate.write_label(&ifend_label)?;
                    gstate.acc_in_use = true;
                    if la != ra {
                        return Err(syntax_error(gstate.compiler_state, "Different alternative types in ?: expression", pos))
                    }
                    Ok(la)
                }
            } else {
                Err(syntax_error(gstate.compiler_state, "Missing alternatives in ?: expression", pos))
            }
        },
        _ => Err(syntax_error(gstate.compiler_state, "Missing alternatives in ?: expression", pos))
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
                        Some(f) => {
                            let acc_in_use = gstate.acc_in_use;
                            if acc_in_use { gstate.write_asm("PHA", 3)?; }
                            
                            if f.bank == gstate.current_bank {
                                gstate.write_asm(&format!("JSR {}", *var), 6)?;
                            } else {
                                if gstate.current_bank == 0 {
                                    // Generate bankswitching call
                                    gstate.write_asm(&format!("JSR Call{}", *var), 6)?;
                                } else {
                                    return Err(syntax_error(gstate.compiler_state, "Banked code can only be called from bank 0 or same bank", pos))
                                }
                            } 
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
        ExprType::Absolute(variable, eight_bits, offset) => {
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
            gstate.flags = FlagsState::Absolute(variable, *eight_bits, *offset);
            Ok(ExprType::Absolute(variable, *eight_bits, *offset))
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

fn generate_neg<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(-*i)),
        _ => {
            let left = ExprType::Immediate(0);
            let right = generate_expr(expr, gstate, pos, false)?;
            generate_arithm(&left, &Operation::Sub(false), &right, gstate, pos, false)
        }
    }
}

fn generate_expr_cond<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    if gstate.acc_in_use {
        gstate.write_asm("PHA", 3)?; 
        gstate.local_label_counter_if += 1;
        let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
        let else_label = format!(".else{}", gstate.local_label_counter_if);
        generate_condition(expr, gstate, pos, false, &else_label)?;
        gstate.write_asm("LDA #0", 2)?;
        gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
        gstate.write_label(&else_label)?;
        gstate.write_asm("LDA #1", 2)?;
        gstate.write_label(&ifend_label)?;
        gstate.write_asm("STA cctmp", 3)?;
        gstate.write_asm("PLA", 3)?;
        Ok(ExprType::Tmp(false))
    } else {
        gstate.local_label_counter_if += 1;
        let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
        let else_label = format!(".else{}", gstate.local_label_counter_if);
        generate_condition(expr, gstate, pos, false, &else_label)?;
        gstate.write_asm("LDA #0", 2)?;
        gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
        gstate.write_label(&else_label)?;
        gstate.write_asm("LDA #1", 2)?;
        gstate.write_label(&ifend_label)?;
        gstate.acc_in_use = true;
        Ok(ExprType::A(false))
    }
}

fn generate_not<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Integer(i) => if *i != 0 {
            Ok(ExprType::Immediate(0))
        } else {
            Ok(ExprType::Immediate(1))
        },
        _ => {
            if gstate.acc_in_use {
                gstate.write_asm("PHA", 3)?; 
                gstate.local_label_counter_if += 1;
                let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
                let else_label = format!(".else{}", gstate.local_label_counter_if);
                generate_condition(expr, gstate, pos, false, &else_label)?;
                gstate.write_asm("LDA #1", 2)?;
                gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
                gstate.write_label(&else_label)?;
                gstate.write_asm("LDA #0", 2)?;
                gstate.write_label(&ifend_label)?;
                gstate.write_asm("STA cctmp", 3)?;
                gstate.write_asm("PLA", 3)?;
                Ok(ExprType::Tmp(false))
            } else {
                gstate.local_label_counter_if += 1;
                let ifend_label = format!(".ifend{}", gstate.local_label_counter_if);
                let else_label = format!(".else{}", gstate.local_label_counter_if);
                generate_condition(expr, gstate, pos, false, &else_label)?;
                gstate.write_asm("LDA #1", 2)?;
                gstate.write_asm(&format!("JMP {}", ifend_label), 3)?;
                gstate.write_label(&else_label)?;
                gstate.write_asm("LDA #0", 2)?;
                gstate.write_label(&ifend_label)?;
                gstate.acc_in_use = true;
                Ok(ExprType::A(false))
            }
        }
    }
}

fn generate_bnot<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(!*i)),
        _ => { 
            let left = generate_expr(expr, gstate, pos, false)?;
            let right = ExprType::Immediate(0xff);
            generate_arithm(&left, &Operation::Xor(false), &right, gstate, pos, false)
        },
    }
}

fn generate_deref<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<ExprType<'a>, Error>
{
    match expr {
        Expr::Identifier((var, sub)) => {
            let v = gstate.compiler_state.get_variable(var);
            if v.var_type == VariableType::CharPtr {
                let sub_output = generate_expr(sub, gstate, pos, false)?;
                match sub_output {
                    ExprType::Nothing => {
                        Ok(ExprType::Absolute(var, true, 0))
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

fn generate_expr<'a>(expr: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize, high_byte: bool) -> Result<ExprType<'a>, Error>
{
    //debug!("Expression: {:?}", expr);
    match expr {
        Expr::Integer(i) => Ok(ExprType::Immediate(*i)),
        Expr::BinOp {lhs, op, rhs} => {
            match op {
                Operation::Assign => {
                    let left = generate_expr(lhs, gstate, pos, high_byte)?;
                    let right = generate_expr(rhs, gstate, pos, high_byte)?;
                    let ret = generate_assign(&left, &right, gstate, pos, high_byte);
                    if !high_byte {
                        match left {
                            ExprType::Absolute(_, eight_bits, _) => {
                                if !eight_bits {
                                    let left = generate_expr(lhs, gstate, pos, true)?;
                                    let right = generate_expr(rhs, gstate, pos, true)?;
                                    generate_assign(&left, &right, gstate, pos, true)?;
                                }
                            },
                            _ => ()
                        };
                    }
                    ret
                },
                Operation::Add(false) | Operation::Sub(false) | Operation::And(false) | Operation::Or(false) | Operation::Xor(false) | Operation::Mul(false) | Operation::Div(false) => {
                    let left = generate_expr(lhs, gstate, pos, high_byte)?;
                    let right = generate_expr(rhs, gstate, pos, high_byte)?;
                    generate_arithm(&left, op, &right, gstate, pos, high_byte)
                },
                Operation::Add(true) | Operation::Sub(true) | Operation::And(true) | Operation::Or(true) | Operation::Xor(true) | Operation::Mul(true) | Operation::Div(true) => {
                    let left = generate_expr(lhs, gstate, pos, high_byte)?;
                    let right = generate_expr(rhs, gstate, pos, high_byte)?;
                    let newright = generate_arithm(&left, op, &right, gstate, pos, high_byte)?;
                    let ret = generate_assign(&left, &newright, gstate, pos, high_byte);
                    if !high_byte {
                        match left {
                            ExprType::Absolute(variable, eight_bits, _) => {
                                let v = gstate.compiler_state.get_variable(variable);
                                if v.var_type == VariableType::Short || (v.var_type == VariableType::CharPtr && !eight_bits) {
                                    let left = generate_expr(lhs, gstate, pos, true)?;
                                    let right = generate_expr(rhs, gstate, pos, true)?;
                                    let newright = generate_arithm(&left, op, &right, gstate, pos, true)?;
                                    generate_assign(&left, &newright, gstate, pos, true)?;
                                }
                            },
                            _ => ()
                        };
                    }
                    ret
                },
                Operation::Eq | Operation::Neq | Operation::Gt | Operation::Gte | Operation::Lt | Operation::Lte | Operation::Land | Operation::Lor => generate_expr_cond(expr, gstate, pos),
                Operation::Bls(true) | Operation::Brs(true) => {
                    let left = generate_expr(lhs, gstate, pos, false)?;
                    let right = generate_expr(rhs, gstate, pos, false)?;
                    let newright = generate_shift(&left, op, &right, gstate, pos)?;
                    generate_assign(&left, &newright, gstate, pos, false)
                },
                Operation::Bls(false) | Operation::Brs(false) => {
                    let left = generate_expr(lhs, gstate, pos, false)?;
                    let right = generate_expr(rhs, gstate, pos, false)?;
                    generate_shift(&left, op, &right, gstate, pos)
                },
                Operation::TernaryCond1 => generate_ternary(lhs, rhs, gstate, pos),
                Operation::TernaryCond2 => unreachable!(),
            }
        },
        Expr::Identifier((var, sub)) => {
            match *var {
                "X" => Ok(ExprType::X),
                "Y" => Ok(ExprType::Y),
                variable => {
                    //debug!("Identifier: {:?}", variable);
                    let v = gstate.compiler_state.get_variable(variable);
                    if let VariableDefinition::Value(val) = &v.def {
                        Ok(ExprType::Immediate(*val))
                    } else {
                        let sub_output = generate_expr(sub, gstate, pos, false)?;
                        match sub_output {
                            ExprType::Nothing => Ok(ExprType::Absolute(variable, v.var_type != VariableType::Short && v.var_type != VariableType::CharPtr, 0)),
                            ExprType::X => Ok(ExprType::AbsoluteX(variable)),
                            ExprType::Y => Ok(ExprType::AbsoluteY(variable)),
                            ExprType::Immediate(v) => Ok(ExprType::Absolute(variable, true, v as u16)),
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
            let expr_type = generate_expr(expr, gstate, pos, high_byte)?;
            generate_plusplus(&expr_type, gstate, pos, false)?;
            Ok(expr_type)
        },
        Expr::PlusPlus(expr, false) => {
            let expr_type = generate_expr(expr, gstate, pos, high_byte)?;
            generate_plusplus(&expr_type, gstate, pos, true)?;
            Ok(expr_type)
        },
        Expr::MinusMinus(expr, true) => {
            let expr_type = generate_expr(expr, gstate, pos, high_byte)?;
            gstate.deferred_plusplus.push((expr_type.clone(), pos, false));
            Ok(expr_type)
        },
        Expr::PlusPlus(expr, true) => {
            let expr_type = generate_expr(expr, gstate, pos, high_byte)?;
            gstate.deferred_plusplus.push((expr_type.clone(), pos, true));
            Ok(expr_type)
        },
        Expr::Neg(v) => generate_neg(v, gstate, pos),
        Expr::Not(v) => generate_not(v, gstate, pos),
        Expr::BNot(v) => generate_bnot(v, gstate, pos),
        Expr::Deref(v) => generate_deref(v, gstate, pos),
        Expr::Nothing => Ok(ExprType::Nothing),
    }
}

fn flags_ok(flags: &FlagsState, expr_type: &ExprType) -> bool
{
    match flags {
        FlagsState::X => *expr_type == ExprType::X,
        FlagsState::Y => *expr_type == ExprType::Y,
        FlagsState::Absolute(var, eight_bits, offset) => *expr_type == ExprType::Absolute(*var, *eight_bits, *offset),
        FlagsState::AbsoluteX(var) => *expr_type == ExprType::AbsoluteX(var),
        FlagsState::AbsoluteY(var) => *expr_type == ExprType::AbsoluteY(var),
        _ => false
    }
}

fn generate_branch_instruction<'a>(op: &Operation, signed: bool, gstate: &mut GeneratorState<'a>, label: &str) -> Result<(), Error>
{
    // Branch instruction
    match op {
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

}

fn generate_condition_ex<'a>(l: &ExprType<'a>, op: &Operation, r: &ExprType<'a>, gstate: &mut GeneratorState<'a>, pos: usize, negate: bool, label: &str) -> Result<(), Error>
{
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
            Operation::Lte => Operation::Gt,
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
        if *v == 0 {
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
                    _ => {
                        if let ExprType::Immediate(v) = left {
                            if (operator == Operation::Neq && *v != 0) || (operator == Operation::Eq && *v == 0) {
                                gstate.write_asm(&format!("JMP {}", label), 3)?;
                            }
                            return Ok(());
                        }
                        return generate_branch_instruction(&operator, true, gstate, label);
                    } 
                }
            } 
        }
    }

    // Compare instruction
    let signed;
    let cmp;
    match left {
        ExprType::Absolute(varname, eight_bits, offset) => {
            if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
            let v = gstate.compiler_state.get_variable(varname);
            if !eight_bits {
                return Err(syntax_error(gstate.compiler_state, "Comparision is not implemented on 16 bits data", pos));
            }
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
                ExprType::Absolute(varname, eight_bits, offset) => {
                    let v = gstate.compiler_state.get_variable(varname);
                    if !eight_bits {
                        return Err(syntax_error(gstate.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                    }
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
                ExprType::Absolute(varname, eight_bits, offset) => {
                    let v = gstate.compiler_state.get_variable(varname);
                    if !eight_bits {
                        return Err(syntax_error(gstate.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                    }
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
            ExprType::Absolute(name, eight_bits, offset) => {
                let v = gstate.compiler_state.get_variable(name);
                if !eight_bits {
                    return Err(syntax_error(gstate.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                }
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

    generate_branch_instruction(&operator, signed, gstate, label)
}

fn generate_condition<'a>(condition: &Expr<'a>, gstate: &mut GeneratorState<'a>, pos: usize, negate: bool, label: &str) -> Result<(), Error>
{
    //debug!("Condition: {:?}", condition);
    match condition {
        Expr::BinOp {lhs, op, rhs} => {
            if match op {
                Operation::Eq => true,
                Operation::Neq => true,
                Operation::Gt => true,
                Operation::Gte => true,
                Operation::Lt => true,
                Operation::Lte => true,
                Operation::Land => {
                    if negate {
                        generate_condition(lhs, gstate, pos, true, label)?;
                        return generate_condition(rhs, gstate, pos, true, label);
                    } else {
                        let ifstart_label = format!(".ifstart{}", gstate.local_label_counter_if);
                        gstate.local_label_counter_if += 1;
                        generate_condition(lhs, gstate, pos, true, &ifstart_label)?;
                        generate_condition(rhs, gstate, pos, false, label)?;
                        gstate.write_label(&ifstart_label)?;
                        return Ok(());
                    }
                },
                Operation::Lor => {
                    if negate {
                        let ifstart_label = format!(".ifstart{}", gstate.local_label_counter_if);
                        gstate.local_label_counter_if += 1;
                        generate_condition(lhs, gstate, pos, false, &ifstart_label)?;
                        generate_condition(rhs, gstate, pos, true, label)?;
                        gstate.write_label(&ifstart_label)?;
                        return Ok(());
                    } else {
                        generate_condition(lhs, gstate, pos, false, label)?;
                        return generate_condition(rhs, gstate, pos, false, label);
                    }
                },
                _ => false,
            } {
                let l = generate_expr(lhs, gstate, pos, false)?;
                let r = generate_expr(rhs, gstate, pos, false)?;
                return generate_condition_ex(&l, op, &r, gstate, pos, negate, label);
            }
        },
        Expr::Not(expr) => {
            return generate_condition(expr, gstate, pos, !negate, label); 
        },
        _ => ()
    };
    
    let cmp;
    let expr = generate_expr(condition, gstate, pos, false)?;
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
                return Ok(());
            },
            ExprType::Absolute(varname, eight_bits, offset) => {
                if gstate.acc_in_use { gstate.write_asm("PHA", 3)?; }
                let v = gstate.compiler_state.get_variable(varname);
                if !eight_bits {
                    return Err(syntax_error(gstate.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                }
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

fn generate_for_loop<'a>(init: &Expr<'a>, condition: &Expr<'a>, update: &Expr<'a>, body: &StatementLoc<'a>, gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    gstate.local_label_counter_for += 1;
    let for_label = format!(".for{}", gstate.local_label_counter_for);
    let forupdate_label = format!(".forupdate{}", gstate.local_label_counter_for);
    let forend_label = format!(".forend{}", gstate.local_label_counter_for);
    generate_expr(init, gstate, pos, false)?;
    gstate.loops.push((forupdate_label.clone(), forend_label.clone()));
    generate_condition(condition, gstate, pos, true, &forend_label)?;
    gstate.write_label(&for_label)?;
    generate_statement(body, gstate)?;
    gstate.write_label(&forupdate_label)?;
    generate_expr(update, gstate, pos, false)?;
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

fn generate_switch<'a>(expr: &Expr<'a>, cases: &Vec<(Vec<i32>, Vec<StatementLoc<'a>>)> , gstate: &mut GeneratorState<'a>, pos: usize) -> Result<(), Error>
{
    let e = generate_expr(expr, gstate, pos, false)?;
    gstate.local_label_counter_if += 1;
    let switchend_label = format!(".switchend{}", gstate.local_label_counter_if);
    match gstate.loops.last() {
        Some(l) => gstate.loops.push((l.0.clone(), switchend_label.clone())),
        None => gstate.loops.push(("".to_string(), switchend_label.clone()))
    }
    gstate.local_label_counter_if += 1;
    let mut switchnextstatement_label = format!(".switchnextstatement{}", gstate.local_label_counter_if);
    for (case, is_last_element) in cases.iter().enumerate().map(|(i,c)| (c, i == cases.len() - 1)) {
        gstate.local_label_counter_if += 1;
        let switchnextcase_label = format!(".switchnextcase{}", gstate.local_label_counter_if);
        let mut jmp_to_next_case = false;
        match case.0.len() {
            0 => (),
            1 => {
                generate_condition_ex(&e, &Operation::Eq, &ExprType::Immediate(case.0[0]), gstate, pos, true, &switchnextcase_label)?;
                jmp_to_next_case = true;
            },
            _ => {
                for i in &case.0 {
                    generate_condition_ex(&e, &Operation::Eq, &ExprType::Immediate(*i), gstate, pos, false, &switchnextstatement_label)?;
                }
                gstate.write_asm(&format!("JMP {}", switchnextcase_label), 3)?;
                jmp_to_next_case = true;
            }
        }
        gstate.write_label(&switchnextstatement_label)?;
        for code in &case.1 {
            generate_statement(code, gstate)?;
        }
        gstate.local_label_counter_if += 1;
        switchnextstatement_label = format!(".switchnextstatement{}", gstate.local_label_counter_if);
        // If this is not the last case...
        if !is_last_element {
            gstate.write_asm(&format!("JMP {}", switchnextstatement_label), 3)?;
        }
        if jmp_to_next_case {
            gstate.write_label(&switchnextcase_label)?;
        }
    }
    gstate.write_label(&switchend_label)?;
    gstate.loops.pop();
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
        None => return Err(syntax_error(gstate.compiler_state, "Continue statement outside loop", pos)),
        Some((cl, _)) => if cl == "" {
            return Err(syntax_error(gstate.compiler_state, "Continue statement outside loop", pos));
        } else {cl}
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
        Statement::Expression(expr) => { 
            generate_expr(&expr, gstate, code.pos, false)?;
        },
        Statement::Block(statements) => {
            for code in statements {
                generate_statement(&code, gstate)?;
            }
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
        Statement::While { condition, body } => { 
            generate_while(condition, body.as_ref(), gstate, code.pos)?; 
        },
        Statement::DoWhile { body, condition } => { 
            generate_do_while(body.as_ref(), condition, gstate, code.pos)?; 
        },
        Statement::Switch { expr, cases } => {
            generate_switch(expr, cases, gstate, code.pos)?;
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
        deferred_plusplus: Vec::new(),
        bankswitching_method: "4K",
        current_bank: 0,
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

    // Try to figure out what is the bankswitching method
    let mut maxbank = 0;
    for f in compiler_state.sorted_functions().iter() {
         if f.1.bank > maxbank { maxbank = f.1.bank; }
    }
    let bankswitching_address: u32;
    if maxbank > 0 {
        bankswitching_address = match maxbank {
            1 => {
                gstate.bankswitching_method = "F8";
                0x1FF8
            },
            2 | 3 => {
                gstate.bankswitching_method = "F6";
                maxbank = 3;
                0x1FF6
            },
            4 | 5 | 6 | 7 => {
                gstate.bankswitching_method = "F4";
                maxbank = 7;
                0x1FF4
            },
            _ => { return Err(Error::Unimplemented { feature: "Bankswitching scheme not implemented" }); },
        };
        gstate.write(&format!("
; Macro that implements Bank Switching trampoline
; X = bank number
; A = hi byte of destination PC
; Y = lo byte of destination PC
        MAC BANK_SWITCH_TRAMPOLINE
        pha     ; push hi byte
        tya     ; Y -> A
        pha     ; push lo byte
        lda ${:04x},x ; do the bank switch
        rts     ; return to target
        ENDM
        ", bankswitching_address))?;
    } else {
        bankswitching_address = 0;
    }
    

    // Generate functions code
    gstate.write("\n; Functions definitions\n\tSEG CODE\n")?;

    let mut nb_banked_functions = 0;
    let mut banked_function_address = 0;

    for bank in 0..=maxbank {
        // Prelude code for each bank
        debug!("Generating code for bank #{}", bank);
        gstate.current_bank = bank;
        gstate.write(&format!("\n\tORG ${}000\n\tRORG $F000\n", bank))?;

        if maxbank > 0 && bank != 0 {
            // Generate trampoline code
            gstate.write("
;----The following code is the same on all banks, except bank 0----
Start
; Ensure that bank 0 is selected
        LDX #$FF
        TXS

        lDA #>(Powerup-1)
        lDY #<(Powerup-1)
        lDX #0
BankSwitch
        BANK_SWITCH_TRAMPOLINE
;----End of bank-identical code----
        ")?;
        }

        // Generate startup code
        if bank == 0 {
            gstate.write("
Powerup
        SEI		; Set the interrupt masking flag in the processor status register.
        CLD		; Clear the BCD mode flag in the processor status register. 
        LDX #$FF	
        TXS

        LDA #0
.loop	  STA $00,X	
        DEX	
        BNE .loop

        JMP main
        ")?;
        }
        
        // Generate functions code
        for f in compiler_state.sorted_functions().iter() {
            if f.1.bank == bank {
                debug!("Generating code for function #{}", f.0);

                gstate.write(&format!("\n{}\tSUBROUTINE\n", f.0))?;
                gstate.local_label_counter_for = 0;
                gstate.local_label_counter_if = 0;
                generate_statement(&f.1.code, &mut gstate)?;
                gstate.write_asm("RTS", 6)?;
            }
        }

        // Generate ROM tables
        gstate.write("\n; Tables in ROM\n")?;
        for v in compiler_state.sorted_variables().iter() {
            if let VariableMemory::ROM(rom_bank) = v.1.memory {
                if rom_bank == bank {
                    match &v.1.def {
                        VariableDefinition::Array(arr) => {
                            if v.1.alignment != 1 {
                                gstate.write(&format!("\n\talign {}\n", v.1.alignment))?;
                            }
                            gstate.write(v.0)?;
                            let mut counter = 0;
                            for i in arr {
                                if counter == 0 || counter == 16 {
                                    gstate.write("\n\thex ")?;
                                }
                                counter += 1;
                                if counter == 16 { counter = 0; }
                                gstate.write(&format!("{:02x}", i))?;
                            } 
                            gstate.write("\n")?;
                        },
                        _ => ()
                    };
                }
            }
        }

        // Epilogue code
        gstate.write(&format!("
        ECHO ([$FFFC-.]d), \"bytes free in bank {}\"
        ", bank))?;

        if bank == 0 {
            // Generate bankswitching functions code
            for f in compiler_state.sorted_functions().iter() {
                if f.1.bank != 0 {
                    nb_banked_functions += 1;
                }
            }
            banked_function_address = 0x0FE0 - nb_banked_functions * 10;
            debug!("Banked function address={:04x}", banked_function_address);
            gstate.write(&format!("
        ORG ${:04x}
        RORG ${:04x}", banked_function_address, 0xF000 + banked_function_address))?;
            for bank_ex in 1..=maxbank {
                for f in compiler_state.sorted_functions().iter() {
                    if f.1.bank == bank_ex {
                        gstate.write(&format!("
Call{}
        LDX ${:04x}+{}
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        RTS", f.0, bankswitching_address, f.1.bank))?;
                    }
                }
            }
        } else {
            for f in compiler_state.sorted_functions().iter() {
                let address = banked_function_address;
                if f.1.bank == bank {
                    debug!("#{} Banked function address={:04x}", bank, banked_function_address);
                    gstate.write(&format!("
        ORG ${:04x}
        RORG ${:04x}
        JSR {}
        LDX ${:04x}
                    ", address + f.1.bank * 0x1000 + 3, 0xF000 + address + 3, f.0, bankswitching_address))?;
                    banked_function_address += 10;
                }
            }
        }

        let starting_code = if maxbank > 0 && bank != 0 { "Start" } else { "Powerup" };
        gstate.write(&format!("
        ORG ${}FFA
        RORG $FFFA

        .word {}\t; NMI
        .word {}\t; RESET
        .word {}\t; IRQ
        \n", bank, starting_code, starting_code, starting_code))?;

    }
 
    gstate.write("\tEND\n")?;
    Ok(())
}
