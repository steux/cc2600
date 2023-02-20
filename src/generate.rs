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
use log::{debug, info};
use std::io::Write;

use crate::error::Error;
use crate::compile::*;
use crate::assemble::{AssemblyCode, AsmMnemonic, AsmMnemonic::*, AsmInstruction};

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum ExprType<'a> {
    Nothing,
    Immediate(i32),
    Tmp(bool),
    Absolute(&'a str, bool, i32), // variable, eight_bits, offset
    AbsoluteX(&'a str),
    AbsoluteY(&'a str),
    A(bool), X, Y,
    Label(&'a str),
}

#[derive(Debug, PartialEq, Copy, Clone)]
pub enum FlagsState<'a> {
    Unknown,
    X, Y,
    Zero, 
    Absolute(&'a str, bool, i32),
}

pub struct GeneratorState<'a> {
    compiler_state: &'a CompilerState<'a>,
    last_included_line_number: usize,
    last_included_position: usize,
    last_included_char: std::str::Chars<'a>,
    writer: &'a mut dyn Write,
    pub local_label_counter_for: u32,
    pub local_label_counter_if: u32,
    local_label_counter_while: u32,
    inline_label_counter: u32,
    loops: Vec<(String,String)>,
    flags: FlagsState<'a>,
    acc_in_use: bool,
    tmp_in_use: bool,
    insert_code: bool,
    deferred_plusplus: Vec<(ExprType<'a>, usize, bool)>,
    pub current_bank: u32,
    pub functions_code: HashMap<String, AssemblyCode>,
    pub current_function: Option<String>,
    bankswitching_scheme: &'a str,
}

impl<'a, 'b> GeneratorState<'a> {
    pub fn new(compiler_state: &'a CompilerState, writer: &'a mut dyn Write, insert_code: bool, bankswitching_scheme: &'a str) -> GeneratorState<'a> {
        GeneratorState {
            compiler_state,
            last_included_line_number: 0,
            last_included_position: 0,
            last_included_char: compiler_state.preprocessed_utf8.chars(),
            writer,
            local_label_counter_for: 0,
            local_label_counter_if: 0,
            local_label_counter_while: 0,
            inline_label_counter: 0,
            loops: Vec::new(),
            flags: FlagsState::Unknown,
            acc_in_use: false,
            tmp_in_use: false,
            insert_code,
            deferred_plusplus: Vec::new(),
            current_bank: 0,
            functions_code: HashMap::new(),
            current_function: None,
            bankswitching_scheme,
        }
    }
    
    fn sasm(&mut self, mnemonic: AsmMnemonic) -> Result<bool, Error>
    {
        self.asm(mnemonic, &ExprType::Nothing, 0, false)
    }

    fn sasm_protected(&mut self, mnemonic: AsmMnemonic) -> Result<bool, Error>
    {
        self.asm(mnemonic, &ExprType::Nothing, 0, true)
    }

    fn asm(&mut self, mnemonic: AsmMnemonic, operand: &ExprType<'b>, pos: usize, high_byte: bool) -> Result<bool, Error>
    {
        let dasm_operand: String;
        let signed;
        let nb_bytes;
        let protected;

        let mut cycles = match mnemonic {
            PHA | PLA => 3,
            INC | DEC => 4,
            RTS => 6,
            _ => 2 
        };

        if *operand != ExprType::Nothing { 
            match operand {
                ExprType::Label(l) => {
                    nb_bytes = match mnemonic {
                        JMP | JSR => 3,
                        _ => 2,
                    };
                    cycles = match mnemonic {
                        JMP => 3,
                        JSR => 6,
                        _ => 2,
                    };
                    signed = false;
                    dasm_operand = (*l).to_string();
                },
                ExprType::Immediate(v) => {
                    nb_bytes = 2;
                    let vx = match high_byte {
                        false => v & 0xff,
                        true => (v >> 8) & 0xff,
                    };
                    signed = false;
                    dasm_operand = format!("#{}", vx);
                },
                ExprType::Tmp(s) => {
                    dasm_operand = "cctmp".to_string();
                    cycles += 1;
                    nb_bytes = 2;
                    signed = *s;
                },
                ExprType::Absolute(variable, eight_bits, off) => {
                    let v = self.compiler_state.get_variable(variable);
                    signed = v.signed;
                    let offset = if v.memory == VariableMemory::Superchip {
                        match mnemonic {
                            STA | STX | STY => *off,
                            _ => off + 0x80
                        }
                    } else if let VariableMemory::MemoryOnChip(_) = v.memory {
                        if self.bankswitching_scheme == "3E" {
                            match mnemonic {
                                STA | STX | STY => off + 0x400,
                                _ => *off
                            }
                        } else { *off }
                    } else { *off };
                    match v.var_type {
                        VariableType::Char => {
                            if high_byte {
                                dasm_operand = "#0".to_string();
                                nb_bytes = 2;
                            } else {
                                if offset > 0 {
                                    dasm_operand = format!("{}+{}", variable, offset);
                                } else {
                                    dasm_operand = variable.to_string();
                                }
                                if v.memory == VariableMemory::Zeropage {
                                    cycles += 1;
                                    nb_bytes = 2;
                                } else {
                                    cycles += 2;
                                    nb_bytes = 3;
                                }
                            }
                        },
                        VariableType::Short => {
                            if *eight_bits && high_byte {
                                dasm_operand = "#0".to_string();
                                nb_bytes = 2;
                            } else {
                                let off = if high_byte { offset + 1 } else { offset };
                                if off != 0 {
                                    dasm_operand = format!("{}+{}", variable, off);
                                } else {
                                    dasm_operand = variable.to_string();
                                }
                                if v.memory == VariableMemory::Zeropage {
                                    cycles += 1;
                                    nb_bytes = 2;
                                } else {
                                    cycles += 2;
                                    nb_bytes = 3;
                                }
                            }
                        },
                        VariableType::CharPtr => if !*eight_bits && v.var_const {
                            if high_byte {
                                if offset != 0 {
                                    dasm_operand = format!("#>{}+{}", variable, offset);
                                } else {
                                    dasm_operand = format!("#>{}", variable);
                                }
                            } else if offset != 0 {
                                dasm_operand = format!("#<{}+{}", variable, offset);
                            } else {
                                dasm_operand = format!("#<{}", variable);
                            }
                            nb_bytes = 2;
                        } else if high_byte && *eight_bits {
                            dasm_operand = "#0".to_string();
                            nb_bytes = 2;
                        } else {
                            let off = if high_byte { offset + 1 } else { offset };
                            if off != 0 {
                                dasm_operand = format!("{}+{}", variable, off);
                            } else {
                                dasm_operand = variable.to_string();
                            }
                            if v.memory == VariableMemory::Zeropage {
                                cycles += 1;
                                nb_bytes = 2;
                            } else {
                                cycles += 2;
                                nb_bytes = 3;
                            }
                        },
                        VariableType::CharPtrPtr | VariableType::ShortPtr => {
                            let v = self.compiler_state.get_variable(variable);
                            let off = offset + if high_byte { v.size as i32 } else { 0 };
                            if off > 0 {
                                dasm_operand = format!("{}+{}", variable, off);
                            } else {
                                dasm_operand = variable.to_string();
                            }
                            cycles += 2;
                            if v.memory == VariableMemory::Zeropage {
                                match mnemonic {
                                    STA | LDA => {
                                        nb_bytes = 3;
                                    },
                                    _ => {
                                        nb_bytes = 2;
                                    }
                                }
                            } else {
                                nb_bytes = 3;
                            }
                        }
                    }
                },
                ExprType::AbsoluteY(variable) => {
                    let v = self.compiler_state.get_variable(variable);
                    signed = v.signed;
                    let offset = if v.memory == VariableMemory::Superchip {
                        match mnemonic {
                            STA | STX | STY => 0,
                            _ => 0x80
                        }
                    } else if let VariableMemory::MemoryOnChip(_) = v.memory {
                        if self.bankswitching_scheme == "3E" {
                            match mnemonic {
                                STA | STX | STY => 0x400,
                                _ => 0 
                            }
                        } else { 0 }
                    } else { 0 };
                    if v.var_type == VariableType::CharPtrPtr || v.var_type == VariableType::ShortPtr {
                        let off = offset + if high_byte { v.size } else { 0 };
                        if off > 0 {
                            dasm_operand = format!("{}+{},Y", variable, off);
                        } else {
                            dasm_operand = format!("{},Y", variable);
                        }
                        cycles += 2;
                        if v.memory == VariableMemory::Zeropage {
                            match mnemonic {
                                STA | LDA => {
                                    nb_bytes = 3;
                                },
                                _ => {
                                    nb_bytes = 2;
                                }
                            }
                        } else {
                            nb_bytes = 3;
                        }
                        match mnemonic {
                            STA => cycles += 1,
                            STY | LDY | CPY => return Err(syntax_error(self.compiler_state, "Can't use Y addressing on Y operation", pos)),
                            CPX => return Err(syntax_error(self.compiler_state, "Can't use Y addressing on compare with X operation", pos)),
                            STX => if v.memory != VariableMemory::Zeropage { return Err(syntax_error(self.compiler_state, "Can't use Y addressing on a non zeropage variable with X storage", pos)) }, 
                            _ => () 
                        }
                    } else if high_byte {
                        dasm_operand = "#0".to_string();
                        nb_bytes = 2;
                    } else if v.var_type == VariableType::CharPtr && !v.var_const {
                        if v.size == 1 {
                            if offset > 0 {
                                dasm_operand = format!("({}+{}),Y", variable, offset);
                            } else {
                                dasm_operand = format!("({}),Y", variable);
                            }
                            if v.memory != VariableMemory::Zeropage {
                                return Err(syntax_error(self.compiler_state, "Y indirect addressing works only on zeropage variables", pos))
                            }
                            nb_bytes = 2;
                            cycles = if mnemonic == STA {6} else {5};
                            match mnemonic {
                                STX | STY | LDX | LDY | CPX | CPY => return Err(syntax_error(self.compiler_state, "Can't use Y indirect addressing on X or Y operation", pos)),
                                _ => () 
                            }
                        } else {
                            return Err(syntax_error(self.compiler_state, "X-Indirect adressing mode not available with Y register", pos));
                        }
                    } else {
                        if offset > 0 {
                            dasm_operand = format!("{}+{},Y", variable, offset);
                        } else {
                            dasm_operand = format!("{},Y", variable);
                        }
                        cycles += 2;
                        if v.memory == VariableMemory::Zeropage {
                            match mnemonic {
                                STA | LDA => {
                                    nb_bytes = 3;
                                },
                                _ => {
                                    nb_bytes = 2;
                                }
                            }
                        } else {
                            nb_bytes = 3;
                        }
                        match mnemonic {
                            STA => cycles += 1,
                            STY | LDY | CPY => return Err(syntax_error(self.compiler_state, "Can't use Y addressing on Y operation", pos)),
                            CPX => return Err(syntax_error(self.compiler_state, "Can't use Y addressing on compare with X operation", pos)),
                            STX => if v.memory != VariableMemory::Zeropage { return Err(syntax_error(self.compiler_state, "Can't use Y addressing on a non zeropage variable with X storage", pos)) }, 
                            _ => () 
                        }
                    }
                },
                ExprType::AbsoluteX(variable) => {
                    let v = self.compiler_state.get_variable(variable);
                    signed = v.signed;
                    let offset = if v.memory == VariableMemory::Superchip {
                        match mnemonic {
                            STA | STX | STY => 0,
                            _ => 0x80
                        }
                    } else if let VariableMemory::MemoryOnChip(_) = v.memory {
                        if self.bankswitching_scheme == "3E" {
                            match mnemonic {
                                STA | STX | STY => 0x400,
                                _ => 0 
                            }
                        } else { 0 }
                    } else { 0 };
                    if v.var_type == VariableType::CharPtr && !v.var_const && v.size == 1 {
                        return Err(syntax_error(self.compiler_state, "Y-Indirect adressing mode not available with X register", pos));
                    }
                    let off = if v.var_type == VariableType::CharPtrPtr || v.var_type == VariableType::ShortPtr {
                        offset + if high_byte { v.size } else { 0 }
                    } else { offset };
                    if high_byte && v.var_type != VariableType::CharPtrPtr && v.var_type != VariableType::ShortPtr {
                        dasm_operand = "#0".to_string();
                        nb_bytes = 2;
                    } else {
                        if off > 0 {
                            dasm_operand = format!("{}+{},X", variable, off);
                        } else {
                            dasm_operand = format!("{},X", variable);
                        }
                        cycles += 2;
                        if v.memory == VariableMemory::Zeropage {
                            nb_bytes = 2;
                        } else {
                            if mnemonic == STA { cycles += 1; }
                            nb_bytes = 3;
                        }
                        match mnemonic {
                            STX | LDX | CPX => return Err(syntax_error(self.compiler_state, "Can't use X addressing on X operation", pos)),
                            CPY => return Err(syntax_error(self.compiler_state, "Can't use X addressing on compare with Y operation", pos)),
                            STY => if v.memory != VariableMemory::Zeropage { return Err(syntax_error(self.compiler_state, "Can't use X addressing on a non zeropage variable with Y storage", pos)) }, 
                            _ => () 
                        }
                    }
                },
                _ => unreachable!()
            }
            protected = false;
        } else {
            dasm_operand = "".to_string();
            signed = false;
            nb_bytes = 1;
            protected = high_byte; // high byte in that case is used as a placeholder
        }
        
        let mut s = mnemonic.to_string();
        if !dasm_operand.is_empty() {
            s += " ";
            s += &dasm_operand;
        }

        if let Some(f) = &self.current_function {
            let code : &mut AssemblyCode = self.functions_code.get_mut(f).unwrap();
            let instruction = AsmInstruction {
                mnemonic, dasm_operand, cycles, nb_bytes, protected,
            };
            code.append_asm(instruction);
        }
        Ok(signed)
    }

    fn inline(&mut self, s: &str) -> Result<(), Error> {
        if let Some(f) = &self.current_function {
            let code : &mut AssemblyCode = self.functions_code.get_mut(f).unwrap();
            code.append_inline(s.to_string());
        }
        Ok(()) 
    } 

    fn comment(&mut self, s: &str) -> Result<(), Error> {
        if let Some(f) = &self.current_function {
            let code : &mut AssemblyCode = self.functions_code.get_mut(f).unwrap();
            code.append_comment(s.trim_end().to_string());
        }
        Ok(()) 
    } 

    fn label(&mut self, l: &str) -> Result<(), Error> {
        if let Some(f) = &self.current_function {
            let code : &mut AssemblyCode = self.functions_code.get_mut(f).unwrap();
            code.append_label(l.to_string());
        }
        Ok(()) 
    } 

    // Inline code
    fn push_code(&mut self, f: &str) -> Result<(), Error> {
        self.inline_label_counter += 1;
        if let Some(fx) = &self.current_function {
            let code2: AssemblyCode = self.functions_code.get(f).unwrap().clone();
            let code : &mut AssemblyCode = self.functions_code.get_mut(fx).unwrap();
            code.append_code(&code2, self.inline_label_counter);
        }
        Ok(()) 
    }

    pub fn write_function(&mut self, f: &str) -> Result<usize, std::io::Error>
    {
        let code: &AssemblyCode = self.functions_code.get(f).unwrap();
        code.write(self.writer, self.insert_code)
    }

    pub fn optimize_function(&mut self, f: &str) -> u32 
    {
        let code: &mut AssemblyCode = self.functions_code.get_mut(f).unwrap();
        let nb = code.optimize();
        if nb > 0 {
            info!("#{} optimized out instructions in function {}", nb, f);
        }
        nb
    }

    pub fn check_branches(&mut self, f: &str) -> u32 
    {
        let code: &mut AssemblyCode = self.functions_code.get_mut(f).unwrap();
        let nb = code.check_branches();
        if nb > 0 {
            info!("#{} too far relative branch fixes in function {}", nb, f);
        }
        nb
    }

    pub fn write(&mut self, s: &str) -> Result<usize, std::io::Error> {
        self.writer.write(s.as_bytes())
    }

    fn purge_deferred_plusplus(&mut self) -> Result<(), Error> {
        let def = self.deferred_plusplus.clone();
        self.deferred_plusplus.clear();
        for d in def {
            self.generate_plusplus(&d.0, d.1, d.2)?;
        }
        Ok(())
    }

    fn generate_included_source_code_line(&mut self, loc: usize) -> Option<&'a str>
    {
        let mut start_of_line = self.last_included_char.clone();
        let mut start_of_line_pos = self.last_included_position;
        if self.last_included_position < loc {
            // Let's find the line including loc
            while self.last_included_position < loc {
                let c = self.last_included_char.next();
                c?; 
                let c = c.unwrap();
                self.last_included_position += 1;
                if c == '\n' { 
                    self.last_included_line_number += 1;
                    start_of_line = self.last_included_char.clone();
                    start_of_line_pos = self.last_included_position;
                }
            };
            // Ok, we have found loc. Let's go to the end of line
            loop {
                let c = self.last_included_char.next();
                if c.is_none() { return Some(start_of_line.as_str()); }
                let c = c.unwrap();
                self.last_included_position += 1;
                if c == '\n' {
                    self.last_included_line_number += 1;
                    return Some(&start_of_line.as_str()[0..(self.last_included_position - start_of_line_pos)]);
                }
            }    
        }
        None
    }

    fn generate_assign(&mut self, left: &ExprType<'a>, right: &ExprType<'a>, pos: usize, high_byte: bool) -> Result<ExprType<'a>, Error>
    {
        match left {
            ExprType::X => {
                match right {
                    ExprType::Immediate(_) => {
                        self.asm(LDX, right, pos, high_byte)?;
                        self.flags = FlagsState::X; 
                        Ok(ExprType::X) 
                    },
                    ExprType::Tmp(_) => {
                        self.asm(LDX, right, pos, high_byte)?;
                        self.flags = FlagsState::X; 
                        self.tmp_in_use = false;
                        Ok(ExprType::X) 
                    },
                    ExprType::Absolute(_, eight_bits, _) => {
                        if !eight_bits {
                            return Err(syntax_error(self.compiler_state, "Can't assign 16 bits data to X", pos));
                        }
                        self.asm(LDX, right, pos, high_byte)?;
                        self.flags = FlagsState::X;
                        Ok(ExprType::X)
                    },
                    ExprType::AbsoluteX(_) => {
                        if self.acc_in_use { self.sasm(PHA)?; }
                        self.asm(LDA, right, pos, high_byte)?;
                        self.sasm(TAX)?;
                        if self.acc_in_use { 
                            self.sasm(PLA)?;
                            self.flags = FlagsState::Unknown;
                        } else {
                            self.flags = FlagsState::X;
                        }
                        Ok(ExprType::X)
                    },
                    ExprType::AbsoluteY(variable) => {
                        let v = self.compiler_state.get_variable(variable);
                        if v.var_type == VariableType::CharPtr && !v.var_const && v.size == 1 {
                            if self.acc_in_use { self.sasm(PHA)?; }
                            self.asm(LDA, right, pos, high_byte)?;
                            self.sasm(TAX)?;
                            if self.acc_in_use { 
                                self.sasm(PLA)?;
                                self.flags = FlagsState::Unknown;
                            } else {
                                self.flags = FlagsState::X;
                            }
                        } else {
                            self.asm(LDX, right, pos, high_byte)?;
                            self.flags = FlagsState::X; 
                        }
                        Ok(ExprType::X)
                    },
                    ExprType::A(_) => {
                        self.sasm(TAX)?;
                        self.flags = FlagsState::X;
                        self.acc_in_use = false;
                        Ok(ExprType::X)
                    },
                    ExprType::X => {
                        Ok(ExprType::X)
                    },
                    ExprType::Y => {
                        if self.acc_in_use { self.sasm(PHA)?; }
                        self.sasm(TYA)?;
                        self.sasm(TAX)?;
                        if self.acc_in_use { 
                            self.sasm(PLA)?;
                            self.flags = FlagsState::Unknown;
                        } else {
                            self.flags = FlagsState::X;
                        }
                        Ok(ExprType::X)
                    },
                    ExprType::Nothing => unreachable!(),
                    ExprType::Label(_) => unreachable!(),
                }
            },
            ExprType::Y => {
                match right {
                    ExprType::Immediate(_) | ExprType::AbsoluteX(_) => {
                        self.asm(LDY, right, pos, high_byte)?;
                        self.flags = FlagsState::Y; 
                        Ok(ExprType::Y) 
                    },
                    ExprType::Tmp(_) => {
                        self.asm(LDY, right, pos, high_byte)?;
                        self.flags = FlagsState::Y; 
                        self.tmp_in_use = false;
                        Ok(ExprType::Y) 
                    },
                    ExprType::Absolute(_, eight_bits, _) => {
                        if !eight_bits {
                            return Err(syntax_error(self.compiler_state, "Can't assign 16 bits data to Y", pos));
                        }
                        self.asm(LDY, right, pos, high_byte)?;
                        self.flags = FlagsState::Y;
                        Ok(ExprType::Y)
                    },
                    ExprType::AbsoluteY(_) => {
                        if self.acc_in_use { self.sasm(PHA)?; }
                        self.asm(LDA, right, pos, high_byte)?;
                        self.sasm(TAY)?;
                        if self.acc_in_use { 
                            self.sasm(PLA)?;
                            self.flags = FlagsState::Unknown;
                        } else {
                            self.flags = FlagsState::Y;
                        }
                        Ok(ExprType::Y)
                    },
                    ExprType::A(_)=> {
                        self.sasm(TAY)?;
                        self.acc_in_use = false;
                        self.flags = FlagsState::Y;
                        Ok(ExprType::Y)
                    },
                    ExprType::X => {
                        if self.acc_in_use { self.sasm(PHA)?; }
                        self.sasm(TXA)?;
                        self.sasm(TAY)?;
                        if self.acc_in_use { 
                            self.sasm(PLA)?;
                            self.flags = FlagsState::Unknown;
                        } else {
                            self.flags = FlagsState::Y;
                        }
                        Ok(ExprType::Y)
                    },
                    ExprType::Y => {
                        self.flags = FlagsState::Y;
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
                            ExprType::Absolute(_, eight_bits, offset) => {
                                self.asm(STX, left, pos, high_byte)?;
                                if !eight_bits {
                                    if *offset == 0 {
                                        if self.acc_in_use { self.sasm(PHA)?; }
                                        self.asm(LDA, &ExprType::Immediate(0), pos, false)?;
                                        self.asm(STA, left, pos, true)?;
                                        if self.acc_in_use { 
                                            self.sasm(PLA)?;
                                            self.flags = FlagsState::Unknown;
                                        } else {
                                            self.flags = FlagsState::Zero;
                                        }
                                    } else {
                                        unreachable!(); 
                                    }
                                }
                                Ok(ExprType::X)
                            },
                            ExprType::AbsoluteX(_) => {
                                if self.acc_in_use { self.sasm(PHA)?; }
                                self.sasm(TXA)?;
                                self.asm(STA, left, pos, high_byte)?;
                                if self.acc_in_use {
                                    self.sasm(PLA)?;
                                    self.flags = FlagsState::Unknown;
                                } else {
                                    self.flags = FlagsState::X;
                                }
                                Ok(ExprType::X)
                            },
                            ExprType::AbsoluteY(variable) => {
                                let v = self.compiler_state.get_variable(variable);
                                if v.memory == VariableMemory::Zeropage {
                                    self.asm(STX, left, pos, high_byte)?;
                                } else {
                                    if self.acc_in_use { self.sasm(PHA)?; }
                                    self.sasm(TXA)?;
                                    self.asm(STA, left, pos, high_byte)?;
                                    if self.acc_in_use {
                                        self.sasm(PLA)?;
                                        self.flags = FlagsState::Unknown;
                                    } else {
                                        self.flags = FlagsState::X;
                                    }
                                }
                                Ok(ExprType::X)
                            },
                            ExprType::A(_) => {
                                if self.acc_in_use {
                                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                                }
                                self.sasm(TXA)?;
                                self.acc_in_use = true;
                                return Ok(ExprType::A(false));
                            },
                            _ => Err(syntax_error(self.compiler_state, "Bad left value in assignement", pos)),
                        }
                    },
                    ExprType::Y => {
                        match left {
                            ExprType::Absolute(_, eight_bits, offset) => {
                                self.asm(STY, left, pos, high_byte)?;
                                if !eight_bits {
                                    if *offset == 0 {
                                        if self.acc_in_use { self.sasm(PHA)?; }
                                        self.asm(LDA, &ExprType::Immediate(0), pos, false)?;
                                        self.asm(STA, left, pos, true)?;
                                        if self.acc_in_use { 
                                            self.sasm(PLA)?;
                                            self.flags = FlagsState::Unknown;
                                        } else {
                                            self.flags = FlagsState::Zero;
                                        }
                                    } else {
                                        unreachable!(); 
                                    }
                                }
                                Ok(ExprType::Y)
                            },
                            ExprType::AbsoluteY(_) => {
                                if self.acc_in_use { self.sasm(PHA)?; }
                                self.sasm(TYA)?;
                                self.asm(STA, left, pos, high_byte)?;
                                if self.acc_in_use {
                                    self.sasm(PLA)?;
                                    self.flags = FlagsState::Unknown;
                                } else {
                                    self.flags = FlagsState::Y;
                                }
                                Ok(ExprType::Y)
                            },
                            ExprType::AbsoluteX(variable) => {
                                let v = self.compiler_state.get_variable(variable);
                                if v.memory == VariableMemory::Zeropage {
                                    self.asm(STY, left, pos, high_byte)?;
                                } else {
                                    if self.acc_in_use { self.sasm(PHA)?; }
                                    self.sasm(TYA)?;
                                    self.asm(STA, left, pos, high_byte)?;
                                    if self.acc_in_use {
                                        self.sasm(PLA)?;
                                        self.flags = FlagsState::Unknown;
                                    } else {
                                        self.flags = FlagsState::Y;
                                    }
                                }
                                Ok(ExprType::Y)
                            },
                            ExprType::A(_) => {
                                if self.acc_in_use {
                                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                                }
                                self.sasm(TYA)?;
                                self.acc_in_use = true;
                                return Ok(ExprType::A(false));
                            },
                            _ => Err(syntax_error(self.compiler_state, "Bad left value in assignement", pos)),
                        }
                    },
                    _ => {
                        let mut acc_in_use = self.acc_in_use;
                        let signed;
                        match right {
                            ExprType::Absolute(_, _, _) | ExprType::AbsoluteX(_) | ExprType::AbsoluteY(_) | ExprType::Immediate(_) | ExprType::Tmp(_) => {
                                if self.acc_in_use { self.sasm(PHA)?; }
                                signed = self.asm(LDA, right, pos, high_byte)?;
                            },
                            ExprType::A(s) => {
                                signed = *s;
                                acc_in_use = false;
                                self.acc_in_use = false;
                            },
                            _ => unreachable!()
                        };
                        match left {
                            ExprType::Absolute(_, _, _) | ExprType::AbsoluteX(_) | ExprType::AbsoluteY(_) => {
                                self.asm(STA, left, pos, high_byte)?;
                                self.flags = FlagsState::Unknown;
                            },
                            ExprType::A(_) => {
                                if acc_in_use {
                                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                                }
                                self.acc_in_use = true;
                                return Ok(ExprType::A(signed));
                            },
                            ExprType::Tmp(_) => {
                                if self.tmp_in_use {
                                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                                }
                                self.tmp_in_use = true;
                                return Ok(ExprType::Tmp(signed));
                            },
                            _ => return Err(syntax_error(self.compiler_state, "Bad left value in assignement", pos)),
                        };
                        if acc_in_use {
                            self.sasm(PLA)?;
                            self.flags = FlagsState::Unknown;
                        }
                        Ok(*left)
                    }
                }
            }
        }
    }

    fn generate_arithm(&mut self, l: &ExprType<'a>, op: &Operation, r: &ExprType<'a>,  pos: usize, high_byte: bool) -> Result<ExprType<'a>, Error>
    {
        let mut acc_in_use = self.acc_in_use;
        debug!("Arithm: {:?},{:?},{:?}", l, op, r);    
        let left;
        let right;

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
        let right2 = match right {
            ExprType::A(s) => {
                if self.tmp_in_use {
                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                }
                self.asm(STA, &ExprType::Tmp(*s), pos, false)?;
                acc_in_use = false;
                self.tmp_in_use = true;
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
                        if acc_in_use { self.sasm(PHA)?; }
                        self.asm(LDA, left, pos, high_byte)?;
                    }
                };
            },
            ExprType::Absolute(variable, eight_bits, off) => {
                if let ExprType::Immediate(r) = right {
                    let v = self.compiler_state.get_variable(variable);
                    if v.var_type == VariableType::CharPtr && !*eight_bits && v.var_const {
                        match op {
                            Operation::Add(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off + *r)),
                            Operation::Sub(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off - *r)),
                            Operation::And(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off & *r)),
                            Operation::Or(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off | *r)),
                            Operation::Xor(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off ^ *r)),
                            Operation::Mul(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off * *r)),
                            Operation::Div(_) => return Ok(ExprType::Absolute(variable, *eight_bits, *off / *r)),
                            _ => (),
                        } 
                    }
                }
                if acc_in_use { self.sasm(PHA)?; }
                self.asm(LDA, left, pos, high_byte)?;
            },
            ExprType::AbsoluteX(_) | ExprType::AbsoluteY(_) => {
                if acc_in_use { self.sasm(PHA)?; }
                self.asm(LDA, left, pos, high_byte)?;
            },
            ExprType::Tmp(_) => {
                if acc_in_use { self.sasm(PHA)?; }
                self.asm(LDA, left, pos, high_byte)?;
                self.tmp_in_use = false;
            },
            ExprType::X => {
                if acc_in_use { self.sasm(PHA)?; }
                self.sasm(TXA)?;
            },
            ExprType::Y => {
                if acc_in_use { self.sasm(PHA)?; }
                self.sasm(TYA)?;
            },
            ExprType::A(_) => {
                acc_in_use = false;
            },
            _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
        }
        self.acc_in_use = true;
        let operation = match op {
            Operation::Add(_) => {
                if !high_byte {
                    self.sasm(CLC)?;
                }
                ADC
            },
            Operation::Sub(_) => {
                if !high_byte {
                    self.sasm(SEC)?;
                }
                SBC
            },
            Operation::And(_) => {
                AND
            },
            Operation::Or(_) => {
                ORA
            },
            Operation::Xor(_) => {
                EOR
            },
            Operation::Mul(_) => { return Err(syntax_error(self.compiler_state, "Operation not possible. 6502 doesn't implement a multiplier.", pos)) },
            Operation::Div(_) => { return Err(syntax_error(self.compiler_state, "Operation not possible. 6502 doesn't implement a divider.", pos)) },
            _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
        };
        let signed;
        match right2 {
            ExprType::Immediate(_) | ExprType::Absolute(_, _, _) | ExprType::AbsoluteX(_) | ExprType::AbsoluteY(_) => {
                signed = self.asm(operation, right2, pos, high_byte)?;
            },
            ExprType::X => {
                signed = false;
                if self.tmp_in_use {
                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                }
                self.asm(STX, &ExprType::Tmp(false), pos, high_byte)?;
                self.asm(operation, &ExprType::Tmp(false), pos, high_byte)?;
            },
            ExprType::Y => {
                signed = false;
                if self.tmp_in_use {
                    return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                }
                self.asm(STY, &ExprType::Tmp(false), pos, high_byte)?;
                self.asm(operation, &ExprType::Tmp(false), pos, high_byte)?;
            },
            ExprType::Tmp(s) => {
                self.asm(operation, right2, pos, high_byte)?;
                self.tmp_in_use = false;
                signed = *s;
            },
            _ => { return Err(Error::Unimplemented { feature: "arithmetics is partially implemented" }); },
        };
        self.flags = FlagsState::Unknown;
        if acc_in_use {
            self.asm(STA, &ExprType::Tmp(false), pos, high_byte)?;
            self.sasm(PLA)?;
            self.tmp_in_use = true;
            Ok(ExprType::Tmp(signed))
        } else {
            Ok(ExprType::A(signed))
        }
    }

    fn generate_shift(&mut self, left: &ExprType<'a>, op: &Operation, right: &ExprType<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        let mut acc_in_use = self.acc_in_use;
        let signed;
        match left {
            ExprType::Immediate(l) => {
                match right {
                    ExprType::Immediate(r) => {
                        match op {
                            Operation::Brs(_) => return Ok(ExprType::Immediate(l >> r)),
                            Operation::Bls(_) => return Ok(ExprType::Immediate(l << r)),
                            _ => unreachable!(),
                        } 
                    },
                    _ => return Err(syntax_error(self.compiler_state, "Incorrect right value for shift operation (constants only)", pos))
                };
            },
            ExprType::Absolute(varname, eight_bits, offset) => {
                let v = self.compiler_state.get_variable(varname);
                if (v.var_type == VariableType::Short || v.var_type == VariableType::ShortPtr || (v.var_type == VariableType::CharPtr && !eight_bits)) && *op == Operation::Brs(false) {
                    // Special shift 8 case for extracting higher byte
                    match right {
                        ExprType::Immediate(value) => {
                            if *value == 8 {
                                if v.var_type == VariableType::CharPtr && !eight_bits && v.var_const {
                                    if acc_in_use { self.sasm(PHA)?; }
                                    signed = self.asm(LDA, left, pos, true)?;
                                    self.flags = FlagsState::Unknown;
                                    if acc_in_use {
                                        self.asm(STA, &ExprType::Tmp(signed), pos, false)?;
                                        self.sasm(PLA)?;
                                        self.tmp_in_use = true;
                                        return Ok(ExprType::Tmp(signed));
                                    } else {
                                        return Ok(ExprType::A(signed));
                                    }
                                } else {
                                    return Ok(ExprType::Absolute(varname, true, offset + v.size as i32));
                                }
                            } else {
                                return Err(syntax_error(self.compiler_state, "Incorrect right value for right shift operation on short (constant 8 only supported)", pos));
                            } 
                        },
                        _ => return Err(syntax_error(self.compiler_state, "Incorrect right value for right shift operation on short (constant 8 only supported)", pos))
                    };
                } else {
                    if acc_in_use { self.sasm(PHA)?; }
                    signed = self.asm(LDA, left, pos, false)?;
                }
            },
            ExprType::AbsoluteX(varname) | ExprType::AbsoluteY(varname) => {
                let v = self.compiler_state.get_variable(varname);
                if (v.var_type == VariableType::ShortPtr || v.var_type == VariableType::CharPtrPtr) && *op == Operation::Brs(false) {
                    // Special shift 8 case for extracting higher byte
                    match right {
                        ExprType::Immediate(value) => {
                            if *value == 8 {
                                if acc_in_use { self.sasm(PHA)?; }
                                signed = self.asm(LDA, left, pos, true)?;
                                self.flags = FlagsState::Unknown;
                                if acc_in_use {
                                    self.asm(STA, &ExprType::Tmp(signed), pos, false)?;
                                    self.sasm(PLA)?;
                                    self.tmp_in_use = true;
                                    return Ok(ExprType::Tmp(signed));
                                } else {
                                    return Ok(ExprType::A(signed));
                                }
                            } else {
                                return Err(syntax_error(self.compiler_state, "Incorrect right value for right shift operation on short (constant 8 only supported)", pos));
                            } 
                        },
                        _ => return Err(syntax_error(self.compiler_state, "Incorrect right value for right shift operation on short (constant 8 only supported)", pos))
                    };
                } else {
                    if acc_in_use { self.sasm(PHA)?; }
                    signed = self.asm(LDA, left, pos, false)?;
                }
            },
            ExprType::X => {
                if acc_in_use { self.sasm(PHA)?; }
                signed = false;
                self.sasm(TXA)?;
            },
            ExprType::Y => {
                if acc_in_use { self.sasm(PHA)?; }
                signed = false;
                self.sasm(TYA)?;
            },
            ExprType::A(s) => { 
                acc_in_use = false;
                signed = *s; 
            },
            ExprType::Tmp(s) => {
                if acc_in_use { self.sasm(PHA)?; }
                signed = *s;
                self.tmp_in_use = false;
                self.asm(LDA, left, pos, false)?;
            },
            _ => return Err(syntax_error(self.compiler_state, "Bad left value for shift operation", pos))
        }
        self.acc_in_use = true;
        let operation = match op {
            Operation::Brs(_) => {
                LSR
            },
            Operation::Bls(_) => {
                ASL
            },
            _ => unreachable!(),
        };
        match right {
            ExprType::Immediate(v) => {
                if *v >= 0 && *v <= 8 {
                    for _ in 0..*v {
                        self.sasm(operation)?;
                    }
                } else {
                    return Err(syntax_error(self.compiler_state, "Negative shift operation not allowed", pos));
                } 
            },
            _ => return Err(syntax_error(self.compiler_state, "Incorrect right value for shift operation (positive constants only)", pos))
        };
        self.flags = FlagsState::Unknown;
        if acc_in_use {
            self.asm(STA, &ExprType::Tmp(signed), pos, false)?;
            self.sasm(PLA)?;
            self.tmp_in_use = true;
            Ok(ExprType::Tmp(signed))
        } else {
            Ok(ExprType::A(signed))
        }
    }

    fn generate_ternary(&mut self, condition: &Expr<'a>, alternatives: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        match alternatives {
            Expr::BinOp {lhs, op, rhs} => {
                if *op == Operation::TernaryCond2 {
                    if self.acc_in_use {
                        self.sasm(PHA)?; 
                        if self.tmp_in_use {
                            return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                        }
                        self.local_label_counter_if += 1;
                        let ifend_label = format!(".ifend{}", self.local_label_counter_if);
                        let else_label = format!(".else{}", self.local_label_counter_if);
                        self.generate_condition(condition, pos, true, &else_label)?;
                        let left = self.generate_expr(lhs, pos, false)?;
                        let la = self.generate_assign(&ExprType::A(false), &left, pos, false)?;
                        self.asm(JMP, &ExprType::Label(&ifend_label), pos, false)?;
                        self.label(&else_label)?;
                        self.acc_in_use = false;
                        let right = self.generate_expr(rhs, pos, false)?;
                        let ra = self.generate_assign(&ExprType::A(false), &right, pos, false)?;
                        self.label(&ifend_label)?;
                        self.asm(STA, &ExprType::Tmp(false), pos, false)?;
                        self.tmp_in_use = true;
                        self.sasm(PLA)?;
                        if la != ra {
                            return Err(syntax_error(self.compiler_state, "Different alternative types in ?: expression", pos))
                        }
                        Ok(ExprType::Tmp(false))
                    } else {
                        self.local_label_counter_if += 1;
                        let ifend_label = format!(".ifend{}", self.local_label_counter_if);
                        let else_label = format!(".else{}", self.local_label_counter_if);
                        self.generate_condition(condition, pos, true, &else_label)?;
                        let left = self.generate_expr(lhs, pos, false)?;
                        let la = self.generate_assign(&ExprType::A(false), &left, pos, false)?;
                        self.asm(JMP, &ExprType::Label(&ifend_label), pos, false)?;
                        self.label(&else_label)?;
                        self.acc_in_use = false;
                        let right = self.generate_expr(rhs, pos, false)?;
                        let ra = self.generate_assign(&ExprType::A(false), &right, pos, false)?;
                        self.label(&ifend_label)?;
                        self.acc_in_use = true;
                        if la != ra {
                            return Err(syntax_error(self.compiler_state, "Different alternative types in ?: expression", pos))
                        }
                        Ok(la)
                    }
                } else {
                    Err(syntax_error(self.compiler_state, "Missing alternatives in ?: expression", pos))
                }
            },
            _ => Err(syntax_error(self.compiler_state, "Missing alternatives in ?: expression", pos))
        }
    }

    fn generate_function_call(&mut self, expr: &Expr<'a>, pos: usize) -> Result<(), Error>
    {
        match expr {
            Expr::Identifier((var, sub)) => {
                match sub.as_ref() {
                    Expr::Nothing => {
                        match self.compiler_state.functions.get(*var) {
                            None => Err(syntax_error(self.compiler_state, "Unknown function identifier", pos)),
                            Some(f) => {
                                let acc_in_use = self.acc_in_use;
                                if acc_in_use { self.sasm(PHA)?; }
                                if f.inline {
                                    if f.code.is_some() {
                                        self.push_code(var)?;
                                    } else {
                                        return Err(syntax_error(self.compiler_state, "Undefined function", pos));
                                    }
                                } else if f.bank == self.current_bank {
                                    self.asm(JSR, &ExprType::Label(var), pos, false)?;
                                } else if self.bankswitching_scheme == "3E" {
                                    if self.current_bank == 0 {
                                        // Generate bankswitching call
                                        self.asm(LDA, &ExprType::Immediate((f.bank - 1) as i32), pos, false)?;
                                        self.asm(STA, &ExprType::Absolute("ROM_SELECT", true, 0), pos, false)?;
                                        self.asm(JSR, &ExprType::Label(var), pos, false)?;
                                    } else {
                                        return Err(syntax_error(self.compiler_state, "Banked code can only be called from bank 0 or same bank", pos))
                                    }
                                } else if self.current_bank == 0 {
                                    // Generate bankswitching call
                                    self.asm(JSR, &ExprType::Label(&format!("Call{}", *var)), pos, false)?;
                                } else {
                                    return Err(syntax_error(self.compiler_state, "Banked code can only be called from bank 0 or same bank", pos))
                                }
                                if acc_in_use { self.sasm(PLA)?; }
                                self.flags = FlagsState::Unknown;
                                Ok(())
                            }
                        }
                    },
                    _ => Err(syntax_error(self.compiler_state, "No subscript allowed here", pos))
                }
            },
            _ => Err(syntax_error(self.compiler_state, "Function call on something else than a function", pos))
        }
    }

    fn generate_plusplus(&mut self, expr_type: &ExprType<'a>, pos: usize, plusplus: bool) -> Result<ExprType<'a>, Error>
    {
        let operation = if plusplus { INC } else { DEC };
        match expr_type {
            ExprType::X => {
                if plusplus {
                    self.sasm(INX)?;
                } else {
                    self.sasm(DEX)?;
                }
                self.flags = FlagsState::X;
                Ok(ExprType::X)
            },
            ExprType::Y => {
                if plusplus {
                    self.sasm(INY)?;
                } else {
                    self.sasm(DEY)?;
                }
                self.flags = FlagsState::Y;
                Ok(ExprType::Y)
            },
            ExprType::Absolute(variable, eight_bits, offset) => {
                let v = self.compiler_state.get_variable(variable);
                match v.memory {
                    VariableMemory::Superchip | VariableMemory::MemoryOnChip(_) => {
                        let op = if plusplus { Operation::Add(false) } else { Operation::Sub(false) };
                        let right = ExprType::Immediate(1);
                        let newright = self.generate_arithm(expr_type, &op, &right, pos, false)?;
                        let ret = self.generate_assign(expr_type, &newright, pos, false);
                        if v.var_type == VariableType::Short || (v.var_type == VariableType::CharPtr && !eight_bits) {
                            let newright = self.generate_arithm(expr_type, &op, &right, pos, true)?;
                            self.generate_assign(expr_type, &newright, pos, true)?;
                        }
                        ret
                    },
                    _ => {
                        self.asm(operation, expr_type, pos, false)?;
                        self.flags = FlagsState::Absolute(variable, *eight_bits, *offset);
                        Ok(ExprType::Absolute(variable, *eight_bits, *offset))
                    }
                } 
            },
            ExprType::AbsoluteX(variable) => {
                self.asm(operation, expr_type, pos, false)?;
                self.flags = FlagsState::Unknown;
                Ok(ExprType::AbsoluteX(variable))
            },
            ExprType::AbsoluteY(_) => {
                let op = if plusplus { Operation::Add(false) } else { Operation::Sub(false) };
                let right = ExprType::Immediate(1);
                let newright = self.generate_arithm(expr_type, &op, &right, pos, false)?;
                self.generate_assign(expr_type, &newright, pos, false)
            },
            _ => {
                if plusplus {
                    Err(syntax_error(self.compiler_state, "Bad left value used with ++ operator", pos))
                } else {
                    Err(syntax_error(self.compiler_state, "Bad left value used with -- operator", pos))
                }
            },
        }
    }

    fn generate_neg(&mut self, expr: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        match expr {
            Expr::Integer(i) => Ok(ExprType::Immediate(-*i)),
            _ => {
                let left = ExprType::Immediate(0);
                let right = self.generate_expr(expr, pos, false)?;
                self.generate_arithm(&left, &Operation::Sub(false), &right, pos, false)
            }
        }
    }

    fn generate_expr_cond(&mut self, expr: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        if self.acc_in_use {
            self.sasm(PHA)?; 
            if self.tmp_in_use {
                return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
            }
            self.local_label_counter_if += 1;
            let ifend_label = format!(".ifend{}", self.local_label_counter_if);
            let else_label = format!(".else{}", self.local_label_counter_if);
            self.generate_condition(expr, pos, false, &else_label)?;
            self.asm(LDA, &ExprType::Immediate(0), pos, false)?;
            self.asm(JMP, &ExprType::Label(&ifend_label), pos, false)?;
            self.label(&else_label)?;
            self.asm(LDA, &ExprType::Immediate(1), pos, false)?;
            self.label(&ifend_label)?;
            self.asm(STA, &ExprType::Tmp(false), pos, false)?;
            self.tmp_in_use = true;
            self.sasm(PLA)?;
            Ok(ExprType::Tmp(false))
        } else {
            self.local_label_counter_if += 1;
            let ifend_label = format!(".ifend{}", self.local_label_counter_if);
            let else_label = format!(".else{}", self.local_label_counter_if);
            self.generate_condition(expr, pos, false, &else_label)?;
            self.asm(LDA, &ExprType::Immediate(0), pos, false)?;
            self.asm(JMP, &ExprType::Label(&ifend_label), pos, false)?;
            self.label(&else_label)?;
            self.asm(LDA, &ExprType::Immediate(1), pos, false)?;
            self.label(&ifend_label)?;
            self.acc_in_use = true;
            Ok(ExprType::A(false))
        }
    }

    fn generate_not(&mut self, expr: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        match expr {
            Expr::Integer(i) => if *i != 0 {
                Ok(ExprType::Immediate(0))
            } else {
                Ok(ExprType::Immediate(1))
            },
            _ => {
                if self.acc_in_use {
                    self.sasm(PHA)?; 
                    self.local_label_counter_if += 1;
                    let ifend_label = format!(".ifend{}", self.local_label_counter_if);
                    let else_label = format!(".else{}", self.local_label_counter_if);
                    self.generate_condition(expr, pos, false, &else_label)?;
                    self.asm(LDA, &ExprType::Immediate(1), pos, false)?;
                    self.asm(JMP, &ExprType::Label(&ifend_label), pos, false)?;
                    self.label(&else_label)?;
                    self.asm(LDA, &ExprType::Immediate(0), pos, false)?;
                    self.label(&ifend_label)?;
                    self.asm(STA, &ExprType::Tmp(false), pos, false)?;
                    self.sasm(PLA)?;
                    Ok(ExprType::Tmp(false))
                } else {
                    self.local_label_counter_if += 1;
                    let ifend_label = format!(".ifend{}", self.local_label_counter_if);
                    let else_label = format!(".else{}", self.local_label_counter_if);
                    self.generate_condition(expr, pos, false, &else_label)?;
                    self.asm(LDA, &ExprType::Immediate(1), pos, false)?;
                    self.asm(JMP, &ExprType::Label(&ifend_label), pos, false)?;
                    self.label(&else_label)?;
                    self.asm(LDA, &ExprType::Immediate(0), pos, false)?;
                    self.label(&ifend_label)?;
                    self.acc_in_use = true;
                    Ok(ExprType::A(false))
                }
            }
        }
    }

    fn generate_bnot(&mut self, expr: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        match expr {
            Expr::Integer(i) => Ok(ExprType::Immediate(!*i)),
            _ => { 
                let left = self.generate_expr(expr, pos, false)?;
                let right = ExprType::Immediate(0xff);
                self.generate_arithm(&left, &Operation::Xor(false), &right, pos, false)
            },
        }
    }

    fn generate_deref(&mut self, expr: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        match expr {
            Expr::Identifier((var, sub)) => {
                let v = self.compiler_state.get_variable(var);
                if v.var_type == VariableType::CharPtr {
                    let sub_output = self.generate_expr(sub, pos, false)?;
                    match sub_output {
                        ExprType::Nothing => {
                            Ok(ExprType::Absolute(var, true, 0))
                        },
                        _ => Err(syntax_error(self.compiler_state, "No subscript is allowed in this context", pos))
                    }
                } else {
                    Err(syntax_error(self.compiler_state, "Deref on something else than a pointer", pos))
                }
            },
            _ => Err(syntax_error(self.compiler_state, "Deref only works on pointers", pos)),
        }
    }

    fn generate_sizeof(&mut self, expr: &Expr<'a>, pos: usize) -> Result<ExprType<'a>, Error>
    {
        match expr {
            Expr::Identifier((var, _)) => {
                let v = self.compiler_state.get_variable(var);
                if v.var_type == VariableType::CharPtr {
                    match &v.def {
                        VariableDefinition::Array(s) => Ok(ExprType::Immediate(s.len() as i32)),
                        _ => Ok(ExprType::Immediate(2)),
                    } 
                } else if v.var_type == VariableType::Char {
                    Ok(ExprType::Immediate(1))
                } else if v.var_type == VariableType::Short {
                    Ok(ExprType::Immediate(2))
                } else {
                    Err(syntax_error(self.compiler_state, "Sizeof only works on variables", pos))
                }
            },
            _ => Err(syntax_error(self.compiler_state, "Sizeof only works on variables", pos)),
        }
    }

    fn generate_expr(&mut self, expr: &Expr<'a>, pos: usize, high_byte: bool) -> Result<ExprType<'a>, Error>
    {
        //debug!("Expression: {:?}", expr);
        match expr {
            Expr::Integer(i) => Ok(ExprType::Immediate(*i)),
            Expr::BinOp {lhs, op, rhs} => {
                match op {
                    Operation::Assign => {
                        let left = self.generate_expr(lhs, pos, high_byte)?;
                        let right = self.generate_expr(rhs, pos, high_byte)?;
                        let ret = self.generate_assign(&left, &right, pos, high_byte);
                        if !high_byte {
                            match left {
                                ExprType::Absolute(_, eight_bits, _) =>  if !eight_bits {
                                    let left = self.generate_expr(lhs, pos, true)?;
                                    let right = self.generate_expr(rhs, pos, true)?;
                                    self.generate_assign(&left, &right, pos, true)?;
                                },
                                ExprType::AbsoluteX(variable) | ExprType::AbsoluteY(variable) => {
                                    let v = self.compiler_state.get_variable(variable);
                                    if v.var_type == VariableType::ShortPtr || v.var_type == VariableType::CharPtrPtr {
                                        let left = self.generate_expr(lhs, pos, true)?;
                                        let right = self.generate_expr(rhs, pos, true)?;
                                        self.generate_assign(&left, &right, pos, true)?;
                                    }
                                },
                                _ => (),
                            };
                        }
                        ret
                    },
                    Operation::Add(false) | Operation::Sub(false) | Operation::And(false) | Operation::Or(false) | Operation::Xor(false) | Operation::Mul(false) | Operation::Div(false) => {
                        let left = self.generate_expr(lhs, pos, high_byte)?;
                        let right = self.generate_expr(rhs, pos, high_byte)?;
                        self.generate_arithm(&left, op, &right, pos, high_byte)
                    },
                    Operation::Add(true) | Operation::Sub(true) | Operation::And(true) | Operation::Or(true) | Operation::Xor(true) | Operation::Mul(true) | Operation::Div(true) => {
                        let left = self.generate_expr(lhs, pos, high_byte)?;
                        let right = self.generate_expr(rhs, pos, high_byte)?;
                        let newright = self.generate_arithm(&left, op, &right, pos, high_byte)?;
                        let ret = self.generate_assign(&left, &newright, pos, high_byte);
                        if !high_byte {
                            match left {
                                ExprType::Absolute(variable, eight_bits, _) => {
                                    let v = self.compiler_state.get_variable(variable);
                                    if v.var_type == VariableType::Short || v.var_type == VariableType::ShortPtr || (v.var_type == VariableType::CharPtr && !eight_bits) {
                                        let left = self.generate_expr(lhs, pos, true)?;
                                        let right = self.generate_expr(rhs, pos, true)?;
                                        let newright = self.generate_arithm(&left, op, &right, pos, true)?;
                                        self.generate_assign(&left, &newright, pos, true)?;
                                    }
                                },
                                ExprType::AbsoluteX(variable) | ExprType::AbsoluteY(variable) => {
                                    let v = self.compiler_state.get_variable(variable);
                                    if v.var_type == VariableType::ShortPtr || v.var_type == VariableType::CharPtrPtr {
                                        let left = self.generate_expr(lhs, pos, true)?;
                                        let right = self.generate_expr(rhs, pos, true)?;
                                        let newright = self.generate_arithm(&left, op, &right, pos, true)?;
                                        self.generate_assign(&left, &newright, pos, true)?;
                                    }
                                },
                                _ => (),
                            };
                        }
                        ret
                    },
                    Operation::Eq | Operation::Neq | Operation::Gt | Operation::Gte | Operation::Lt | Operation::Lte | Operation::Land | Operation::Lor => self.generate_expr_cond(expr, pos),
                    Operation::Bls(true) | Operation::Brs(true) => {
                        let left = self.generate_expr(lhs, pos, false)?;
                        let right = self.generate_expr(rhs,pos, false)?;
                        let newright = self.generate_shift(&left, op, &right, pos)?;
                        self.generate_assign(&left, &newright, pos, false)
                    },
                    Operation::Bls(false) | Operation::Brs(false) => {
                        let left = self.generate_expr(lhs, pos, false)?;
                        let right = self.generate_expr(rhs, pos, false)?;
                        self.generate_shift(&left, op, &right, pos)
                    },
                    Operation::TernaryCond1 => self.generate_ternary(lhs, rhs, pos),
                    Operation::TernaryCond2 => unreachable!(),
                }
            },
            Expr::Identifier((var, sub)) => {
                match *var {
                    "X" => Ok(ExprType::X),
                    "Y" => Ok(ExprType::Y),
                    variable => {
                        let v = self.compiler_state.get_variable(variable);
                        let sub_output = self.generate_expr(sub, pos, false)?;
                        match sub_output {
                            ExprType::Nothing => if let VariableDefinition::Value(val) = &v.def {
                                Ok(ExprType::Immediate(*val))
                            } else {
                                Ok(ExprType::Absolute(variable, v.var_type == VariableType::Char, 0))
                            },
                            ExprType::X => Ok(ExprType::AbsoluteX(variable)),
                            ExprType::Y => Ok(ExprType::AbsoluteY(variable)),
                            ExprType::Immediate(val) => Ok(ExprType::Absolute(variable, v.var_type != VariableType::CharPtrPtr && v.var_type != VariableType::ShortPtr, val)),
                            _ => Err(syntax_error(self.compiler_state, "Subscript not allowed (only X, Y and constants are allowed)", pos))
                        }
                    },
                }
            },
            Expr::FunctionCall(expr) => {
                self.generate_function_call(expr, pos)?;
                Ok(ExprType::Nothing)
            },
            Expr::MinusMinus(expr, false) => {
                let expr_type = self.generate_expr(expr, pos, high_byte)?;
                self.generate_plusplus(&expr_type, pos, false)?;
                Ok(expr_type)
            },
            Expr::PlusPlus(expr, false) => {
                let expr_type = self.generate_expr(expr, pos, high_byte)?;
                self.generate_plusplus(&expr_type, pos, true)?;
                Ok(expr_type)
            },
            Expr::MinusMinus(expr, true) => {
                let expr_type = self.generate_expr(expr, pos, high_byte)?;
                self.deferred_plusplus.push((expr_type, pos, false));
                Ok(expr_type)
            },
            Expr::PlusPlus(expr, true) => {
                let expr_type = self.generate_expr(expr, pos, high_byte)?;
                self.deferred_plusplus.push((expr_type, pos, true));
                Ok(expr_type)
            },
            Expr::Neg(v) => self.generate_neg(v, pos),
            Expr::Not(v) => self.generate_not(v, pos),
            Expr::BNot(v) => self.generate_bnot(v, pos),
            Expr::Deref(v) => self.generate_deref(v, pos),
            Expr::Sizeof(v) => self.generate_sizeof(v, pos),
            Expr::Nothing => Ok(ExprType::Nothing),
        }
    }

    fn generate_branch_instruction(&mut self, op: &Operation, signed: bool, label: &str) -> Result<(), Error>
    {
        // Branch instruction
        match op {
            Operation::Neq => {
                self.asm(BNE, &ExprType::Label(label), 0, false)?;
                Ok(())
            },
            Operation::Eq => {
                self.asm(BEQ, &ExprType::Label(label), 0, false)?;
                Ok(())
            },
            Operation::Lt => {
                if signed {
                    self.asm(BMI, &ExprType::Label(label), 0, false)?;
                } else {
                    self.asm(BCC, &ExprType::Label(label), 0, false)?;
                } 
                Ok(())
            },
            Operation::Gt => {
                self.local_label_counter_if += 1;
                let label_here = format!(".ifhere{}", self.local_label_counter_if);
                self.asm(BEQ, &ExprType::Label(&label_here), 0, false)?;
                if signed {
                    self.asm(BPL, &ExprType::Label(label), 0, false)?;
                } else {
                    self.asm(BCS, &ExprType::Label(label), 0, false)?;
                }
                self.label(&label_here)?;
                Ok(())
            },
            Operation::Lte => {
                if signed {
                    self.asm(BMI, &ExprType::Label(label), 0, false)?;
                    self.asm(BEQ, &ExprType::Label(label), 0, false)?;
                } else {
                    self.asm(BCC, &ExprType::Label(label), 0, false)?;
                    self.asm(BEQ, &ExprType::Label(label), 0, false)?;
                } 
                Ok(())
            },
            Operation::Gte => {
                if signed {
                    self.asm(BPL, &ExprType::Label(label), 0, false)?;
                } else {
                    self.asm(BCS, &ExprType::Label(label), 0, false)?;
                } 
                Ok(())
            },
            _ => Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
        }
    }

    fn generate_condition_ex(&mut self, l: &ExprType<'a>, op: &Operation, r: &ExprType<'a>, pos: usize, negate: bool, label: &str) -> Result<(), Error>
    {
        let left;
        let right;

        let switch = match &l {
            ExprType::X | ExprType::Y => {
                left = l; right = r;
                false
            }, 
            _ => match &r {
                ExprType::A(_) => {
                    left = r; right = l;
                    true 
                },
                _ => {
                    left = l; right = r;
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
            if v == 0 {
                // Let's see if we can shortcut compare instruction 
                if flags_ok(&self.flags, left) {
                    match operator {
                        Operation::Neq => {
                            self.asm(BNE, &ExprType::Label(label), pos, false)?;
                            return Ok(());
                        },
                        Operation::Eq => {
                            self.asm(BEQ, &ExprType::Label(label), pos, false)?;
                            return Ok(());
                        },
                        _ => {
                            if let ExprType::Immediate(v) = left {
                                if (operator == Operation::Neq && *v != 0) || (operator == Operation::Eq && *v == 0) {
                                    self.asm(JMP, &ExprType::Label(label), pos, false)?;
                                }
                                return Ok(());
                            }
                            return self.generate_branch_instruction(&operator, true, label);
                        } 
                    }
                } 
            }
        }

        // Compare instruction
        let signed;
        let cmp;
        match left {
            ExprType::Absolute(_, eight_bits, _) => {
                if self.acc_in_use { self.sasm(PHA)?; }
                if !eight_bits {
                    return Err(syntax_error(self.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                }
                signed = self.asm(LDA, left, pos, false)?;
                cmp = true;
            },
            ExprType::AbsoluteX(_) | ExprType::AbsoluteY(_)=> {
                if self.acc_in_use { self.sasm(PHA)?; }
                signed = self.asm(LDA, left, pos, false)?;
                cmp = true;
            },
            ExprType::A(sign) => {
                cmp = true;
                signed = *sign;
                self.acc_in_use = false;
            },
            ExprType::Tmp(sign) => {
                if self.acc_in_use { self.sasm(PHA)?; }
                signed = *sign;
                self.asm(LDA, left, pos, false)?;
                self.tmp_in_use = false;
                cmp = true;
            },
            ExprType::Y => {
                signed = false;
                match right {
                    ExprType::Immediate(_) => {
                        self.asm(CPY, right, pos, false)?;
                        cmp = false;
                    },
                    ExprType::Absolute(_, eight_bits, _) => {
                        if !eight_bits {
                            return Err(syntax_error(self.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                        }
                        self.asm(CPY, right, pos, false)?;
                        cmp = false;
                    },
                    ExprType::A(s) => {
                        if self.tmp_in_use {
                            return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                        }
                        self.asm(STA, &ExprType::Tmp(*s), pos, false)?;
                        self.asm(CPY, &ExprType::Tmp(*s), pos, false)?;
                        cmp = false;
                        self.acc_in_use = false;
                    },
                    ExprType::Tmp(_) => {
                        self.asm(CPY, right, pos, false)?;
                        cmp = false;
                        self.tmp_in_use = false;
                    },
                    _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
                } 
            },
            ExprType::X => {
                signed = false;
                match right {
                    ExprType::Immediate(_) => {
                        self.asm(CPX, right, pos, false)?;
                        cmp = false;
                    },
                    ExprType::Absolute(_, eight_bits, _) => {
                        if !eight_bits {
                            return Err(syntax_error(self.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                        }
                        self.asm(CPX, right, pos, false)?;
                        cmp = false;
                    },
                    ExprType::A(s) => {
                        if self.tmp_in_use {
                            return Err(syntax_error(self.compiler_state, "Code too complex for the compiler", pos))
                        }
                        self.asm(STA, &ExprType::Tmp(*s), pos, false)?;
                        self.asm(CPX, &ExprType::Tmp(*s), pos, false)?;
                        cmp = false;
                        self.acc_in_use = false;
                    },
                    ExprType::Tmp(_) => {
                        self.asm(CPX, right, pos, false)?;
                        cmp = false;
                        self.tmp_in_use = false;
                    },
                    _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
                } 
            },
            _ => { return Err(Error::Unimplemented { feature: "condition statement is partially implemented" }); },
        }

        if cmp {
            match right {
                ExprType::Immediate(_) => {
                    self.asm(CMP, right, pos, false)?;
                },
                ExprType::Absolute(_, eight_bits, _) => {
                    if !eight_bits {
                        return Err(syntax_error(self.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                    }
                    self.asm(CMP, right, pos, false)?;
                },
                ExprType::AbsoluteX(_) => {
                    self.asm(CMP, right, pos, false)?;
                },
                ExprType::AbsoluteY(_) => {
                    self.asm(CMP, right, pos, false)?;
                },
                _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
            } 
            if self.acc_in_use { 
                self.sasm(PLA)?; 
            }
        }

        self.generate_branch_instruction(&operator, signed, label)
    }

    fn generate_condition(&mut self, condition: &Expr<'a>, pos: usize, negate: bool, label: &str) -> Result<(), Error>
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
                                self.generate_condition(lhs, pos, true, label)?;
                                return self.generate_condition(rhs, pos, true, label);
                            } else {
                                let ifstart_label = format!(".ifstart{}", self.local_label_counter_if);
                                self.local_label_counter_if += 1;
                                self.generate_condition(lhs, pos, true, &ifstart_label)?;
                                self.generate_condition(rhs, pos, false, label)?;
                                self.label(&ifstart_label)?;
                                return Ok(());
                            }
                        },
                        Operation::Lor => {
                            if negate {
                                let ifstart_label = format!(".ifstart{}", self.local_label_counter_if);
                                self.local_label_counter_if += 1;
                                self.generate_condition(lhs, pos, false, &ifstart_label)?;
                                self.generate_condition(rhs, pos, true, label)?;
                                self.label(&ifstart_label)?;
                                return Ok(());
                            } else {
                                self.generate_condition(lhs, pos, false, label)?;
                                return self.generate_condition(rhs, pos, false, label);
                            }
                        },
                        _ => false,
                } {
                    let l = self.generate_expr(lhs, pos, false)?;
                    let r = self.generate_expr(rhs, pos, false)?;
                    return self.generate_condition_ex(&l, op, &r, pos, negate, label);
                }
            },
            Expr::Not(expr) => {
                return self.generate_condition(expr, pos, !negate, label); 
            },
            _ => ()
        };

        let expr = self.generate_expr(condition, pos, false)?;
        if flags_ok(&self.flags, &expr) {
            if negate {
                self.asm(BEQ, &ExprType::Label(label), pos, false)?;
            } else {
                self.asm(BNE, &ExprType::Label(label), pos, false)?;
            }
            Ok(())
        } else {
            self.flags = FlagsState::Unknown;
            match expr {
                ExprType::Immediate(v) => {
                    if v != 0 {
                        if !negate {
                            self.asm(JMP, &ExprType::Label(label), pos, false)?;
                        }
                    } else if negate {
                        self.asm(JMP, &ExprType::Label(label), pos, false)?;
                    }
                    return Ok(());
                },
                ExprType::Absolute(_, eight_bits, _) => {
                    if self.acc_in_use { self.sasm(PHA)?; }
                    if !eight_bits {
                        return Err(syntax_error(self.compiler_state, "Comparision is not implemented on 16 bits data", pos));
                    }
                    self.asm(LDA, &expr, pos, false)?;
                },
                ExprType::AbsoluteX(_) | ExprType::AbsoluteY(_) => {
                    if self.acc_in_use { self.sasm(PHA)?; }
                    self.asm(LDA, &expr, pos, false)?;
                },
                ExprType::A(_) => {
                    self.acc_in_use = false;
                },
                ExprType::Y => {
                    self.asm(CPY, &ExprType::Immediate(0), pos, false)?;
                },
                ExprType::X => {
                    self.asm(CPX, &ExprType::Immediate(0), pos, false)?;
                }
                ExprType::Tmp(_) => {
                    if self.acc_in_use { self.sasm(PHA)?; }
                    self.asm(LDA, &expr, pos, false)?;
                    self.tmp_in_use = false;
                },
                _ => return Err(Error::Unimplemented { feature: "condition statement is partially implemented" })
            }

            if negate {
                self.asm(BEQ, &ExprType::Label(label), 0, false)?;
            } else {
                self.asm(BNE, &ExprType::Label(label), 0, false)?;
            }
            Ok(())
        }
    }

    fn generate_for_loop(&mut self, init: &Expr<'a>, condition: &Expr<'a>, update: &Expr<'a>, body: &StatementLoc<'a>, pos: usize) -> Result<(), Error>
    {
        self.local_label_counter_for += 1;
        let for_label = format!(".for{}", self.local_label_counter_for);
        let forupdate_label = format!(".forupdate{}", self.local_label_counter_for);
        let forend_label = format!(".forend{}", self.local_label_counter_for);
        self.generate_expr(init, pos, false)?;
        self.loops.push((forupdate_label.clone(), forend_label.clone()));
        self.generate_condition(condition, pos, true, &forend_label)?;
        self.label(&for_label)?;
        self.generate_statement(body)?;
        self.label(&forupdate_label)?;
        self.generate_expr(update, pos, false)?;
        self.purge_deferred_plusplus()?;
        self.generate_condition(condition, pos, false, &for_label)?;
        self.label(&forend_label)?;
        self.loops.pop();
        Ok(())
    }

    fn generate_if(&mut self, condition: &Expr<'a>, body: &StatementLoc<'a>, else_body: Option<&StatementLoc<'a>>, pos: usize) -> Result<(), Error>
    {
        self.local_label_counter_if += 1;
        let ifend_label = format!(".ifend{}", self.local_label_counter_if);
        match else_body {
            None => {
                match body.statement {
                    Statement::Break => {
                        let brk_label = {
                            match self.loops.last() {
                                None => return Err(syntax_error(self.compiler_state, "Break statement outside loop", pos)),
                                Some((_, bl)) => bl.clone(),
                            }
                        };
                        self.generate_condition(condition, pos, false, &brk_label)?;
                    },
                    Statement::Continue => {
                        let cont_label = {
                            match self.loops.last() {
                                None => return Err(syntax_error(self.compiler_state, "Break statement outside loop", pos)),
                                Some((cl, _)) => cl.clone(),
                            }
                        };
                        self.generate_condition(condition, pos, false, &cont_label)?;
                    },
                    _ => {
                        self.generate_condition(condition, pos, true, &ifend_label)?;
                        self.generate_statement(body)?;
                        self.label(&ifend_label)?;
                    }

                }
            },
            Some(else_statement) => {
                let else_label = format!(".else{}", self.local_label_counter_if);
                self.generate_condition(condition, pos, true, &else_label)?;
                let saved_flags = self.flags;
                self.generate_statement(body)?;
                self.asm(JMP, &ExprType::Label(&ifend_label), 0, false)?;
                self.label(&else_label)?;
                self.flags = saved_flags;
                self.generate_statement(else_statement)?;
                self.label(&ifend_label)?;
            }
        };
        Ok(())
    }

    fn generate_switch(&mut self, expr: &Expr<'a>, cases: &Vec<(Vec<i32>, Vec<StatementLoc<'a>>)>, pos: usize) -> Result<(), Error>
    {
        let e = self.generate_expr(expr, pos, false)?;
        self.local_label_counter_if += 1;
        let switchend_label = format!(".switchend{}", self.local_label_counter_if);
        match self.loops.last() {
            Some(l) => self.loops.push((l.0.clone(), switchend_label.clone())),
            None => self.loops.push(("".to_string(), switchend_label.clone()))
        }
        self.local_label_counter_if += 1;
        let mut switchnextstatement_label = format!(".switchnextstatement{}", self.local_label_counter_if);
        for (case, is_last_element) in cases.iter().enumerate().map(|(i,c)| (c, i == cases.len() - 1)) {
            self.local_label_counter_if += 1;
            let switchnextcase_label = format!(".switchnextcase{}", self.local_label_counter_if);
            let mut jmp_to_next_case = false;
            match case.0.len() {
                0 => (),
                1 => {
                    self.generate_condition_ex(&e, &Operation::Eq, &ExprType::Immediate(case.0[0]), pos, true, &switchnextcase_label)?;
                    jmp_to_next_case = true;
                },
                _ => {
                    for i in &case.0 {
                        self.generate_condition_ex(&e, &Operation::Eq, &ExprType::Immediate(*i), pos, false, &switchnextstatement_label)?;
                    }
                    self.asm(JMP, &ExprType::Label(&switchnextcase_label), 0, false)?;
                    jmp_to_next_case = true;
                }
            }
            self.label(&switchnextstatement_label)?;
            for code in &case.1 {
                self.generate_statement(code)?;
            }
            self.local_label_counter_if += 1;
            switchnextstatement_label = format!(".switchnextstatement{}", self.local_label_counter_if);
            // If this is not the last case...
            if !is_last_element {
                self.asm(JMP, &ExprType::Label(&switchnextstatement_label), 0, false)?;
            }
            if jmp_to_next_case {
                self.label(&switchnextcase_label)?;
            }
        }
        self.label(&switchend_label)?;
        self.loops.pop();
        Ok(())
    }

    fn generate_while(&mut self, condition: &Expr<'a>, body: &StatementLoc<'a>, pos: usize) -> Result<(), Error>
    {
        if let Statement::Expression(Expr::Nothing) = body.statement {
            self.generate_do_while(body, condition, pos)
        } else {
            self.local_label_counter_while += 1;
            let while_label = format!(".while{}", self.local_label_counter_while);
            let whileend_label = format!(".whileend{}", self.local_label_counter_while);
            self.loops.push((while_label.clone(), whileend_label.clone()));
            self.label(&while_label)?;
            self.generate_condition(condition, pos, true, &whileend_label)?;
            self.generate_statement(body)?;
            self.asm(JMP, &ExprType::Label(&while_label), pos, false)?;
            self.label(&whileend_label)?;
            self.loops.pop();
            Ok(())
        }
    }

    fn generate_do_while(&mut self, body: &StatementLoc<'a>, condition: &Expr<'a>, pos: usize) -> Result<(), Error>
    {
        self.local_label_counter_while += 1;
        let dowhile_label = format!(".dowhile{}", self.local_label_counter_while);
        let dowhilecondition_label = format!(".dowhilecondition{}", self.local_label_counter_while);
        let dowhileend_label = format!(".dowhileend{}", self.local_label_counter_while);
        self.loops.push((dowhilecondition_label.clone(), dowhileend_label.clone()));
        self.label(&dowhile_label)?;
        self.generate_statement(body)?;
        self.label(&dowhilecondition_label)?;
        self.generate_condition(condition, pos, false, &dowhile_label)?;
        self.label(&dowhileend_label)?;
        self.loops.pop();
        Ok(())
    }

    fn generate_break(&mut self, pos: usize) -> Result<(), Error>
    {
        let brk_label;
        {
            brk_label = match self.loops.last() {
                None => return Err(syntax_error(self.compiler_state, "Break statement outside loop", pos)),
                Some((_, bl)) => bl.clone(),
            };
        }
        self.asm(JMP, &ExprType::Label(&brk_label), pos, false)?;
        Ok(())
    }

    fn generate_continue(&mut self, pos: usize) -> Result<(), Error>
    {
        let cont_label = match self.loops.last() {
            None => return Err(syntax_error(self.compiler_state, "Continue statement outside loop", pos)),
            Some((cl, _)) => if cl.is_empty() {
                return Err(syntax_error(self.compiler_state, "Continue statement outside loop", pos));
            } else {cl.clone()}
        };
        self.asm(JMP, &ExprType::Label(&cont_label), pos, false)?;
        Ok(())
    }

    pub fn generate_return(&mut self) -> Result<(), Error>
    {
        self.sasm(RTS)?; 
        Ok(())
    }

    fn generate_asm_statement(&mut self, s: &'a str) -> Result<(), Error>
    {
        self.inline(s)?;
        Ok(())
    }

    fn generate_goto_statement(&mut self, s: &'a str ) -> Result<(), Error>
    {
        self.asm(JMP, &ExprType::Label(&format!(".{}", s)), 0, false)?;
        Ok(())
    }

    fn generate_strobe_statement(&mut self, expr: &Expr<'a>, pos: usize) -> Result<(), Error>
    {
        match expr {
            Expr::Identifier((name, _)) => {
                let v = self.compiler_state.get_variable(name);
                match v.var_type {
                    VariableType::CharPtr => {
                        self.asm(STA, &ExprType::Absolute(name, true, 0), pos, false)?;
                        Ok(())
                    },
                    _ => Err(syntax_error(self.compiler_state, "Strobe only works on memory pointers", pos)),
                }
            },
            _ => Err(syntax_error(self.compiler_state, "Strobe only works on memory pointers", pos)),
        }
    }

    fn generate_csleep_statement(&mut self, cycles: i32, pos: usize) -> Result<(), Error>
    {
        match cycles {
            2 => self.sasm(NOP)?,
            3 => self.asm(STA, &ExprType::Absolute("DUMMY", true, 0), pos, false)?,
            4 => {
                self.sasm(NOP)?;
                self.sasm(NOP)?
            },
            5 => self.asm(DEC, &ExprType::Absolute("DUMMY", true, 0), pos, false)?,
            6 => {
                self.sasm(NOP)?;
                self.sasm(NOP)?;
                self.sasm(NOP)?
            },
            7 => {
                self.sasm_protected(PHA)?;
                self.sasm_protected(PLA)?
            }
            8 => {
                self.sasm(NOP)?;
                self.sasm(NOP)?;
                self.sasm(NOP)?;
                self.sasm(NOP)?
            }
            9 => {
                self.asm(DEC, &ExprType::Absolute("DUMMY", true, 0), pos, false)?;
                self.sasm(NOP)?;
                self.sasm(NOP)?
            },
            10 => {
                self.asm(DEC, &ExprType::Absolute("DUMMY", true, 0), pos, false)?;
                self.asm(DEC, &ExprType::Absolute("DUMMY", true, 0), pos, false)?
            },
            _ => return Err(syntax_error(self.compiler_state, "Unsupported cycle sleep value", pos))
        };
        Ok(())
    }

    fn generate_load_store_statement(&mut self, expr: &ExprType<'a>, pos: usize, load: bool) -> Result<(), Error>
    {
        match expr {
            ExprType::X => self.asm(if load {TXA} else {TAX}, &ExprType::Nothing, pos, false)?,
            ExprType::Y => self.asm(if load {TYA} else {TAY}, &ExprType::Nothing, pos, false)?,
            _ => self.asm(if load {LDA} else {STA}, expr, pos, false)?,
        };
        Ok(())
    }

    pub fn generate_statement(&mut self, code: &StatementLoc<'a>) -> Result<(), Error>
    {
        // Include C source code into generated asm
        // debug!("{:?}, {}, {}, {}", expr, pos, self.last_included_position, self.last_included_line_number);
        if self.insert_code {
            let included_source_code = self.generate_included_source_code_line(code.pos);
            let line_to_be_written = included_source_code.map(|line| line.to_string());
            // debug!("{:?}, {}, {}", line_to_be_written, self.last_included_position, self.last_included_line_number);
            if let Some(l) = line_to_be_written {
                self.comment(&l)?; // Should include the '\n'
            }
        }

        self.purge_deferred_plusplus()?;

        self.acc_in_use = false;
        self.tmp_in_use = false;

        if let Some(label) = &code.label {
            self.label(&format!(".{}", label))?;
        }

        // Generate different kind of statements
        match &code.statement {
            Statement::Expression(expr) => { 
                self.generate_expr(expr, code.pos, false)?;
            },
            Statement::Block(statements) => {
                for code in statements {
                    self.generate_statement(code)?;
                }
            },
            Statement::For { init, condition, update, body } => { 
                self.generate_for_loop(init, condition, update, body.as_ref(), code.pos)?; 
            },
            Statement::If { condition, body, else_body } => { 
                match else_body {
                    None => self.generate_if(condition, body.as_ref(), None, code.pos)?,
                    Some(ebody) => self.generate_if(condition, body.as_ref(), Some(ebody.as_ref()), code.pos)?,
                }; 
            },
            Statement::While { condition, body } => { 
                self.generate_while(condition, body.as_ref(), code.pos)?; 
            },
            Statement::DoWhile { body, condition } => { 
                self.generate_do_while(body.as_ref(), condition, code.pos)?; 
            },
            Statement::Switch { expr, cases } => {
                self.generate_switch(expr, cases, code.pos)?;
            },
            Statement::Break => { self.generate_break(code.pos)?; }
            Statement::Continue => { self.generate_continue(code.pos)?; }
            Statement::Return => { self.generate_return()?; }
            Statement::Asm(s) => { self.generate_asm_statement(s)?; }
            Statement::Strobe(s) => { self.generate_strobe_statement(s, code.pos)?; }
            Statement::Store(e) => { 
                let param = self.generate_expr(e, code.pos, false)?;
                self.generate_load_store_statement(&param, code.pos, false)?; 
            }
            Statement::Load(e) => { 
                let param = self.generate_expr(e, code.pos, false)?;
                self.generate_load_store_statement(&param, code.pos, true)?; 
            }
            Statement::CSleep(s) => { self.generate_csleep_statement(*s, code.pos)?; }
            Statement::Goto(s) => { self.generate_goto_statement(s)?; }
        }

        self.purge_deferred_plusplus()?;
        Ok(())
    }
}


fn flags_ok(flags: &FlagsState, expr_type: &ExprType) -> bool
{
    match flags {
        FlagsState::X => *expr_type == ExprType::X,
        FlagsState::Y => *expr_type == ExprType::Y,
        FlagsState::Absolute(var, eight_bits, offset) => *expr_type == ExprType::Absolute(var, *eight_bits, *offset),
        _ => false
    }
}
