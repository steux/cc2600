use std::io::Write;
use std::fmt::{self, Debug};

#[derive(Debug, Copy, Clone, PartialEq)]
pub enum AsmMnemonic {
    LDA, LDX, LDY,
    STA, STX, STY,
    TAX, TAY, TXA, TYA,
    ADC, SBC, EOR, AND, ORA,
    LSR, ASL,
    CLC, SEC,  
    CMP, CPX, CPY, 
    BCC, BCS, BEQ, BMI, BNE, BPL,
    INC, INX, INY,
    DEC, DEX, DEY,
    JMP, JSR, RTS,
    PHA, PLA,
}

impl fmt::Display for AsmMnemonic {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        fmt::Debug::fmt(self, f)
    }
}

#[derive(Debug)]
pub struct AsmInstruction {
    pub mnemonic: AsmMnemonic,
    pub dasm_operand: String,
    pub cycles: u32,
    pub nb_bytes: u32
}

#[derive(Debug)]
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
                    if inst.dasm_operand.len() > 0 {
                        s += writer.write(&format!("\t{} {:19}\t; {}\n", inst.mnemonic.to_string(), &inst.dasm_operand, inst.cycles).as_bytes())?;
                    } else {
                        s += writer.write(&format!("\t{:23}\t; {}\n", inst.mnemonic.to_string(), inst.cycles).as_bytes())?;
                    }
                } else {
                    if inst.dasm_operand.len() > 0 {
                        s += writer.write(&format!("\t{} {}\n", inst.mnemonic.to_string(), &inst.dasm_operand).as_bytes())?;
                    } else {
                        s += writer.write(&format!("\t{}\n", inst.mnemonic.to_string()).as_bytes())?;
                    }
                }
            },
            AsmLine::Inline(inst) => {
                s += writer.write(&format!("\t{}\n", inst).as_bytes())?;
            },
            AsmLine::Comment(comment) => {
                s += writer.write(&format!(";{}\n", comment).as_bytes())?;
            },
            AsmLine::Dummy => (),
        }
        Ok(s)
    }
}

pub struct AssemblyCode {
    code: Vec<AsmLine>
}

impl AssemblyCode {
    pub fn new() -> AssemblyCode {
        AssemblyCode {
            code: Vec::<AsmLine>::new()
        }
    }
    pub fn append_asm(&mut self, inst: AsmInstruction) -> () {
        self.code.push(AsmLine::Instruction(inst));
    }
    pub fn append_inline(&mut self, s: String) -> () {
        self.code.push(AsmLine::Inline(s));
    }
    pub fn append_label(&mut self, s: String) -> () {
        self.code.push(AsmLine::Label(s));
    }
    pub fn append_comment(&mut self, s: String) -> () {
        self.code.push(AsmLine::Comment(s));
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
        let mut iter = self.code.iter_mut();
        let mut first = iter.next();
        loop {
            match &first {
                None => return removed_instructions,
                Some(AsmLine::Instruction(_)) => break,
                _ => first = iter.next(),
            }
        }

        let mut second = iter.next();
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
                    },
                    _ => second = iter.next(),
                }
            }

            if let Some(AsmLine::Instruction(i1)) = &first {
                if let Some(AsmLine::Instruction(i2)) = &second {
                    // Remove PLA/PHA pairs
                    remove_both = i1.mnemonic == AsmMnemonic::PLA && i2.mnemonic == AsmMnemonic::PHA;
                    // Remove STA followed by LDA
                    remove_second = i1.mnemonic == AsmMnemonic::STA && i2.mnemonic == AsmMnemonic::LDA && i1.dasm_operand == i2.dasm_operand;
                }
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
}
