use std::io::Write;
use std::fmt::{self, Debug};
use log::{debug, error};
use crate::compile::Operation;

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum AsmMnemonic {
    LDA, LDX, LDY,
    STA, STX, STY,
    TAX, TAY, TXA, TYA,
    ADC, SBC, EOR, AND, ORA,
    LSR, ASL, ROR,
    CLC, SEC,  
    CMP, CPX, CPY, 
    BCC, BCS, BEQ, BMI, BNE, BPL,
    INC, INX, INY,
    DEC, DEX, DEY,
    JMP, JSR, RTS,
    PHA, PLA,
    #[cfg(constant_time)]
    PHP, 
    #[cfg(constant_time)]
    PLP,
    NOP
}

impl fmt::Display for AsmMnemonic {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self, f)
    }
}

#[derive(Debug, Clone)]
pub struct AsmInstruction {
    pub mnemonic: AsmMnemonic,
    pub dasm_operand: String,
    pub cycles: u32,
    pub cycles_alt: Option<u32>,
    pub nb_bytes: u32,
    pub protected: bool,
}

#[derive(Debug, Clone)]
enum AsmLine {
    Label(String),
    Instruction(AsmInstruction),
    Inline(String),
    Comment(String),
    Dummy,
}

impl AsmLine {
    fn write(&self, writer: &mut dyn Write, cycles: bool) -> Result<usize, std::io::Error> {
        let mut s = 0;
        match self {
            AsmLine::Label(string) => { 
                s += writer.write(string.as_bytes())?;
                s += writer.write("\n".as_bytes())?;
            },
            AsmLine::Instruction(inst) => {
                if cycles {
                    let c = if let Some(alt) = inst.cycles_alt {
                        format!("\t; {}/{}", inst.cycles, alt)
                    } else {
                        format!("\t; {}", inst.cycles)
                    };
                    if !inst.dasm_operand.is_empty() {
                        s += writer.write(format!("\t{} {:19}{}\n", inst.mnemonic, &inst.dasm_operand, c).as_bytes())?;
                    } else {
                        s += writer.write(format!("\t{:23}{}\n", inst.mnemonic, c).as_bytes())?;
                    }
                } else {
                    if !inst.dasm_operand.is_empty() {
                        s += writer.write(format!("\t{} {}\n", inst.mnemonic, &inst.dasm_operand).as_bytes())?;
                    } else {
                        s += writer.write(format!("\t{}\n", inst.mnemonic).as_bytes())?;
                    }
                }
            },
            AsmLine::Inline(inst) => {
                s += writer.write(format!("\t{}\n", inst).as_bytes())?;
            },
            AsmLine::Comment(comment) => {
                s += writer.write(format!(";{}\n", comment).as_bytes())?;
            },
            AsmLine::Dummy => (),
        }
        Ok(s)
    }
}

#[derive(Debug, Clone)]
pub struct AssemblyCode {
    code: Vec<AsmLine>
}

impl AssemblyCode {
    pub fn new() -> AssemblyCode {
        AssemblyCode {
            code: Vec::<AsmLine>::new()
        }
    }
    pub fn append_asm(&mut self, inst: AsmInstruction) {
        self.code.push(AsmLine::Instruction(inst));
    }
    pub fn append_inline(&mut self, s: String) {
        self.code.push(AsmLine::Inline(s));
    }
    pub fn append_label(&mut self, s: String) {
        self.code.push(AsmLine::Label(s));
    }
    pub fn append_comment(&mut self, s: String) {
        self.code.push(AsmLine::Comment(s));
    }
    // For inlined code: modify local labels in each instruction
    pub fn append_code(&mut self, code: &AssemblyCode, inline_counter: u32)  {
        for i in &code.code {
            match i {
                AsmLine::Label(l) => self.code.push(AsmLine::Label(format!("{}inline{}", l, inline_counter))),
                AsmLine::Instruction(inst) => {
                    match inst.mnemonic {
                        AsmMnemonic::BCC | AsmMnemonic::BCS | AsmMnemonic::BEQ | AsmMnemonic::BMI | AsmMnemonic::BNE | AsmMnemonic::BPL | AsmMnemonic::JMP => {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: inst.mnemonic,
                                dasm_operand: format!("{}inline{}", inst.dasm_operand, inline_counter),
                                cycles: inst.cycles,
                                cycles_alt: inst.cycles_alt,
                                nb_bytes: inst.nb_bytes,
                                protected: false,
                            }));
                        },
                        _ => self.code.push(i.clone()),
                    }
                },
                _ => self.code.push(i.clone()),
            }
        }
        //self.code.extend_from_slice(&code.code);
    }
    pub fn write(&self, writer: &mut dyn Write, cycles: bool) -> Result<usize, std::io::Error> {
        let mut s = 0;
        for i in &self.code {
            s += i.write(writer, cycles)?;
        }
        Ok(s)
    }

    pub fn optimize(&mut self) -> u32 {
        let mut removed_instructions = 0u32;
        let mut accumulator = None;
        let mut x_register = None;
        let mut y_register = None;
        let mut iter = self.code.iter_mut();
        let mut first = iter.next();
        let mut second = iter.next();

        loop {
            match &first {
                None => return removed_instructions,
                Some(AsmLine::Instruction(_)) => break,
                _ => first = iter.next(),
            }
        }
        // Analyze the first instruction to check for a load
        if let Some(AsmLine::Instruction(inst)) = &first {
            if inst.dasm_operand.starts_with('#') {
                if inst.mnemonic == AsmMnemonic::LDA {
                    if let Ok(v) = inst.dasm_operand[1..].parse::<i32>() {
                        accumulator = Some(v);
                    }
                } else if inst.mnemonic == AsmMnemonic::LDX {
                    if let Ok(v) = inst.dasm_operand[1..].parse::<i32>() {
                        x_register = Some(v);
                    }
                } else if inst.mnemonic == AsmMnemonic::LDY {
                    if let Ok(v) = inst.dasm_operand[1..].parse::<i32>() {
                        y_register = Some(v);
                    }
                }
            }
        } else { unreachable!(); }

        loop {
            // For each iteration of this loop, first must point to an Instruction
            // and second point to the next asm line
            let mut remove_both = false;
            let mut remove_second = false;
            
            // Make sure second points also to an instruction
            loop {
                match &second {
                    None => return removed_instructions,
                    Some(AsmLine::Instruction(_)) => break,
                    Some(AsmLine::Label(_)) => {
                        // If this is a label, restart
                        first = iter.next();
                        loop {
                            match &first {
                                None => return removed_instructions,
                                Some(AsmLine::Instruction(_)) => break,
                                _ => first = iter.next(),
                            }
                        }
                        second = iter.next();
                        // Reset known register values
                        accumulator = None;
                        x_register = None;
                        y_register = None;
                        // Analyze the first instruction to check for a load
                        if let Some(AsmLine::Instruction(inst)) = &first {
                            if inst.dasm_operand.starts_with('#') {
                                if inst.mnemonic == AsmMnemonic::LDA {
                                    if let Ok(v) = inst.dasm_operand[1..].parse::<i32>() {
                                        accumulator = Some(v);
                                    }
                                } else if inst.mnemonic == AsmMnemonic::LDX {
                                    if let Ok(v) = inst.dasm_operand[1..].parse::<i32>() {
                                        x_register = Some(v);
                                    }
                                } else if inst.mnemonic == AsmMnemonic::LDY {
                                    if let Ok(v) = inst.dasm_operand[1..].parse::<i32>() {
                                        y_register = Some(v);
                                    }
                                }
                            }
                        } else { unreachable!(); }
                    },
                    _ => second = iter.next(),
                }
            }

            // Analyze pairs of instructions
            if let Some(AsmLine::Instruction(i1)) = &first {
                if let Some(AsmLine::Instruction(i2)) = &second {
                    // Remove PLA/PHA pairs
                    if i1.mnemonic == AsmMnemonic::PLA && i2.mnemonic == AsmMnemonic::PHA && !i1.protected && !i2.protected {
                        remove_both = true;
                    }
                    // Remove STA followed by LDA
                    if i1.mnemonic == AsmMnemonic::STA && i2.mnemonic == AsmMnemonic::LDA && i1.dasm_operand == i2.dasm_operand && !i2.protected {
                        remove_second = true;
                    }
                    // Check CMP and remove the branck if the result is obvious
                    if let Some(r) = accumulator {
                        if i1.mnemonic == AsmMnemonic::CMP && i1.dasm_operand.starts_with('#') {
                            if let Ok(v) = i1.dasm_operand[1..].parse::<i32>() {
                                // The result IS obvious
                                match i2.mnemonic {
                                    AsmMnemonic::BNE => if r == v {
                                        remove_both = true;
                                    },
                                    AsmMnemonic::BEQ => if r != v {
                                        remove_both = true;
                                    },
                                    _ => ()
                                }
                            }
                        }
                    }
                    if let Some(r) = x_register {
                        if i1.mnemonic == AsmMnemonic::CPX && i1.dasm_operand.starts_with('#') {
                            if let Ok(v) =i1.dasm_operand[1..].parse::<i32>() {
                                // The result IS obvious
                                match i2.mnemonic {
                                    AsmMnemonic::BNE => if r == v {
                                        remove_both = true;
                                    },
                                    AsmMnemonic::BEQ => if r != v {
                                        remove_both = true;
                                    },
                                    _ => ()
                                }
                            }
                        }
                    }
                    if let Some(r) = y_register {
                        if i1.mnemonic == AsmMnemonic::CPY && i1.dasm_operand.starts_with('#') {
                            if let Ok(v) = i1.dasm_operand[1..].parse::<i32>() {
                                // The result IS obvious
                                match i2.mnemonic {
                                    AsmMnemonic::BNE => if r == v {
                                        remove_both = true;
                                    },
                                    AsmMnemonic::BEQ => if r != v {
                                        remove_both = true;
                                    },
                                    _ => ()
                                }
                            }
                        }
                    }
                } else { unreachable!() };
            } else { unreachable!() };

            if !remove_second {
                // Analyze the second instruction to check for a load
                if let Some(AsmLine::Instruction(inst)) = &second {
                    match inst.mnemonic {
                        AsmMnemonic::LDA => {
                            if inst.dasm_operand.starts_with('#') {
                                match inst.dasm_operand[1..].parse::<i32>() {
                                    Ok(v) => {
                                        if let Some(v2) = accumulator {
                                            if v == v2 {
                                                // Remove this instruction
                                                remove_second = !inst.protected;
                                            } else {
                                                accumulator = Some(v);
                                            }
                                        } else {
                                            accumulator = Some(v);
                                        } 
                                    },
                                    _ => accumulator = None,
                                }
                            } else {
                                accumulator = None;
                            } 
                        },
                        AsmMnemonic::LDX => {
                            if inst.dasm_operand.starts_with('#') {
                                match inst.dasm_operand[1..].parse::<i32>() {
                                    Ok(v) => {
                                        if let Some(v2) = x_register {
                                            if v == v2 {
                                                // Remove this instruction
                                                remove_second = !inst.protected;
                                            } else {
                                                x_register = Some(v);
                                            }
                                        } else {
                                            x_register = Some(v);
                                        } 
                                    },
                                    _ => x_register = None,
                                }
                            } else {
                                x_register = None;
                            } 
                        },
                        AsmMnemonic::LDY => {
                            if inst.dasm_operand.starts_with('#') {
                                match inst.dasm_operand[1..].parse::<i32>() {
                                    Ok(v) => {
                                        if let Some(v2) = y_register {
                                            if v == v2 {
                                                // Remove this instruction
                                                remove_second = !inst.protected;
                                            } else {
                                                y_register = Some(v);
                                            }
                                        } else {
                                            y_register = Some(v);
                                        }
                                    },
                                    _ => y_register = None,
                                }
                            } else {
                                y_register = None;
                            } 
                        },
                        AsmMnemonic::INX | AsmMnemonic::DEX  => x_register = None,
                        AsmMnemonic::INY | AsmMnemonic::DEY  => y_register = None,
                        AsmMnemonic::TAX => x_register = accumulator,
                        AsmMnemonic::TAY => y_register = accumulator,
                        AsmMnemonic::TXA => accumulator = x_register,
                        AsmMnemonic::TYA => accumulator = y_register,
                        AsmMnemonic::ADC | AsmMnemonic::SBC | AsmMnemonic::EOR | AsmMnemonic::AND | AsmMnemonic::ORA => accumulator = None,
                        AsmMnemonic::LSR | AsmMnemonic::ASL => accumulator = None,
                        AsmMnemonic::PLA | AsmMnemonic::PHA => accumulator = None,
                        AsmMnemonic::JSR | AsmMnemonic::JMP => accumulator = None,
                        _ => ()
                    }
                } else { unreachable!(); }
            }

            if remove_both {
                *first.unwrap() = AsmLine::Dummy;
                *second.unwrap() = AsmLine::Dummy;
                removed_instructions += 2;
                first = iter.next();
                loop {
                    match &first {
                        None => return removed_instructions,
                        Some(AsmLine::Instruction(_)) => break,
                        _ => first = iter.next(),
                    }
                }
                // Reset known register values
                // This is not optimal, since theoretically this instruction is in the flow of instructions.
                accumulator = None;
                x_register = None;
                y_register = None;
                // Analyze the first instruction to check for a load
                if let Some(AsmLine::Instruction(inst)) = &first {
                    if inst.dasm_operand.starts_with('#') {
                        if inst.mnemonic == AsmMnemonic::LDA {
                            accumulator = Some(inst.dasm_operand[1..].parse::<i32>().unwrap());
                        }
                        if inst.mnemonic == AsmMnemonic::LDX {
                            x_register = Some(inst.dasm_operand[1..].parse::<i32>().unwrap());
                        }
                        if inst.mnemonic == AsmMnemonic::LDY {
                            y_register = Some(inst.dasm_operand[1..].parse::<i32>().unwrap());
                        }
                    }
                } else { unreachable!(); }
                second = iter.next();
            } else if remove_second {
                *second.unwrap() = AsmLine::Dummy;
                removed_instructions += 1;
                second = iter.next();
            } else {
                first = second;
                second = iter.next();
            }
        }
    }
    
    pub fn check_branches(&mut self) -> u32 {
        // Loop until there is no problematic branch instruction
        let mut restart = true;
        let mut nb_fixes = 0;
        debug!("Code: {:?}", self);
        while restart {
            // Check each branch instruction one after each other
            // Let's find the first one
            let mut position = 0;
            let mut i = self.code.iter();
            let mut repair = false;
            loop {
                let j = i.next();
                if j.is_none() { restart = false; break; }
                if let Some(AsmLine::Instruction(inst)) = j {
                    match inst.mnemonic {
                        AsmMnemonic::BEQ | AsmMnemonic::BNE | AsmMnemonic::BMI |
                        AsmMnemonic::BPL | AsmMnemonic::BCS | AsmMnemonic::BCC => {
                            // Ok, let's try to find the label above and under and try to count the bytes
                            let mut bytes_above = 0;
                            let mut bytes_below = 0;
                            let mut index_above = position;
                            let mut index_below = position + 1;
                            let mut reached_above = false;
                            let above;
                            let mut notfound = 0;
                            loop {
                                if !reached_above {
                                    match &self.code[index_above] {
                                        AsmLine::Label(l) => {
                                            debug!("Iter above: {:?}", l);
                                            if *l == inst.dasm_operand {
                                                above = true;
                                                break;
                                            } 
                                        },
                                        AsmLine::Instruction(k) => {
                                            debug!("Iter above: {:?}", k);
                                            bytes_above += k.nb_bytes;
                                        },
                                        _ => ()
                                    }
                                }
                                match self.code.get(index_below) {
                                    Some(AsmLine::Label(l)) => {
                                        debug!("Iter below: {:?}", l);
                                        if *l == inst.dasm_operand {
                                            above = false;
                                            break;
                                        } 
                                    },
                                    Some(AsmLine::Instruction(k)) => {
                                        debug!("Iter below: {:?}", k);
                                        bytes_below += k.nb_bytes;
                                    },
                                    None => notfound |= 2,
                                    _ => ()
                                }
                                if index_above == 0 {
                                    reached_above = true;
                                    notfound |= 1;
                                } else {
                                    index_above -= 1;
                                }
                                index_below += 1;
                                if notfound == 3 { 
                                    error!("Label {} not found", inst.dasm_operand);
                                    unreachable!() 
                                };
                            }
                            // Ok, now we have the distance in bytes
                            let distance = if above { bytes_above } else { bytes_below };
                            //error!("distance = {:?}", distance);
                            //if above {unreachable!();}
                            if distance > 127 {
                                // OK. We have a problem here
                                // This branch should be changed for a jump
                                repair = true;
                                break;
                            }
                        },
                        _ => ()
                    }
                }
                position += 1;
            }
            if repair {
                nb_fixes += 1;
                // Identifies branch operation
                let signed;
                let mut remove = 1;
                let label;
                let operation = if let AsmLine::Instruction(inst) = &self.code[position] {
                    signed = (inst.mnemonic == AsmMnemonic::BPL) || (inst.mnemonic == AsmMnemonic::BMI);
                    label = inst.dasm_operand.clone();
                    match inst.mnemonic {
                        AsmMnemonic::BNE => Operation::Neq,
                        AsmMnemonic::BEQ => Operation::Eq,
                        AsmMnemonic::BMI => if let Some(AsmLine::Instruction(inst2)) = self.code.get(position + 1) {
                                if inst2.mnemonic == AsmMnemonic::BEQ && inst2.dasm_operand == inst.dasm_operand { remove = 2; Operation::Lte } else { Operation::Lt }
                            } else { Operation::Lt },
                        AsmMnemonic::BCC => if let Some(AsmLine::Instruction(inst2)) = self.code.get(position + 1) {
                                if inst2.mnemonic == AsmMnemonic::BEQ && inst2.dasm_operand == inst.dasm_operand { remove = 2; Operation::Lte } else { Operation::Lt }
                            } else { Operation::Lt },
                        AsmMnemonic::BPL => Operation::Gte,
                        AsmMnemonic::BCS => Operation::Gte,
                        _ => unreachable!()
                    }
                } else {
                    unreachable!();
                };
                // Negate the operation
                let operation2 = match operation {
                    Operation::Eq => Operation::Neq,
                    Operation::Neq => Operation::Eq,
                    Operation::Gt => Operation::Lte,
                    Operation::Gte => Operation::Lt,
                    Operation::Lt => Operation::Gte,
                    Operation::Lte => Operation::Gt,
                    _ => unreachable!()
                };
                let mut tail = self.code.split_off(position + remove);
                self.code.truncate(position);
                let label2 = format!(".fix{}", nb_fixes);
                match operation2 {
                    Operation::Eq => self.code.push(AsmLine::Instruction(AsmInstruction {
                        mnemonic: AsmMnemonic::BEQ, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                    })),
                    Operation::Neq => self.code.push(AsmLine::Instruction(AsmInstruction {
                        mnemonic: AsmMnemonic::BNE, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                    })),
                    Operation::Gt => {
                        let label3 = format!(".fixup{}", nb_fixes);
                        self.code.push(AsmLine::Instruction(AsmInstruction {
                            mnemonic: AsmMnemonic::BEQ, dasm_operand: label3.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                        }));       
                        if signed {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: AsmMnemonic::BPL, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                            }));       
                        } else {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: AsmMnemonic::BCS, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                            }));       
                        }
                        self.code.push(AsmLine::Label(label3));
                    },
                    Operation::Gte => {
                        if signed {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: AsmMnemonic::BPL, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                            }));       
                        } else {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: AsmMnemonic::BCS, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                            }));       
                        }
                    },
                    Operation::Lt => {
                        if signed {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: AsmMnemonic::BMI, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                            }));       
                        } else {
                            self.code.push(AsmLine::Instruction(AsmInstruction {
                                mnemonic: AsmMnemonic::BCC, dasm_operand: label2.clone(), cycles: 2, cycles_alt: Some(3), nb_bytes: 2, protected: false
                            }));       
                        }
                    },
                    _ => unreachable!()
                }
                self.code.push(AsmLine::Instruction(AsmInstruction {
                    mnemonic: AsmMnemonic::JMP, dasm_operand: label, cycles: 3, cycles_alt: None, nb_bytes: 3, protected: false
                }));
                self.code.push(AsmLine::Label(label2));
                self.code.append(&mut tail);
            }
        }
        nb_fixes
    }
}
