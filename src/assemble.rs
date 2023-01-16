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

impl AsmInstruction {
}

#[derive(Debug)]
enum AsmLine {
    Label(String),
    Instruction(AsmInstruction),
    Inline(String),
    Comment(String)
}

impl AsmLine {
    fn write(&self, writer: &mut dyn Write) -> Result<usize, std::io::Error> {
        let mut s = 0;
        match self {
            AsmLine::Label(string) => { 
                s += writer.write(string.as_bytes())?;
                s += writer.write("\n".as_bytes())?;
            },
            AsmLine::Instruction(inst) => {
                s += writer.write(&format!("\t{} {:19}\t; {} cycles\n", inst.mnemonic.to_string(), &inst.dasm_operand, inst.cycles).as_bytes())?;
            },
            AsmLine::Inline(inst) => {
                s += writer.write(inst.as_bytes())?;
            },
            AsmLine::Comment(comment) => {
                s += writer.write(&format!(";{}", comment).as_bytes())?;
            },
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
    pub fn append(&mut self, inst: AsmInstruction) -> () {
        self.code.push(AsmLine::Instruction(inst));
    }
    fn write(&self, writer: &mut dyn Write) -> Result<usize, std::io::Error> {
        let mut s = 0;
        for i in &self.code {
            s += i.write(writer)?;
        }
        Ok(s)
    } 
}
