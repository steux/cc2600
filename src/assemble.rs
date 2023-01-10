use crate::compile::*;
use crate::generate::ExprType;

use std::io::Write;
use std::fmt::{self, Debug, Display};

#[derive(Debug, PartialEq)]
enum AsmMnemonic {
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
struct AsmInstruction<'a> {
    mnemonic: AsmMnemonic,
    operand: ExprType<'a>,
    high_byte: bool,
}

impl<'a> AsmInstruction<'a> {
    fn to_dasm(&self, compiler_state: &'a CompilerState<'a>) -> String {
        let mut s = self.mnemonic.to_string();
        if self.operand != ExprType::Nothing { 
            s += " ";
            match self.operand {
                ExprType::Absolute(variable, eight_bits, offset) => {
                    let v = compiler_state.get_variable(variable);
                    match v.var_type {
                        VariableType::Char => if offset > 0 {
                            s += &format!("{}+{}", variable, offset);
                        } else {
                            s += &format!("{}", variable);
                        },
                        VariableType::Short => if self.high_byte {
                            s += &format!("{} + 1", variable);
                        } else {
                            s += &format!("{}", variable);
                        },
                        VariableType::CharPtr => if v.var_type == VariableType::CharPtr && !eight_bits && v.var_const {
                            if self.high_byte {
                                s += &format!("#>{}", variable);
                            } else {
                                s += &format!("#<{}", variable);
                            }
                        } else {
                            if self.high_byte && eight_bits {
                                s += "#0";
                            } else {
                                let off = if self.high_byte { offset + 1 } else { offset };
                                if off > 0 {
                                    s += &format!("{}+{}", variable, off);
                                } else {
                                    s += &format!("{}", variable);
                                }
                            }
                        },
                    }
                },
                _ => unreachable!()
            }
        }
        s
    }
    fn cycles(&self, compiler_state: &'a CompilerState<'a>) -> u8 {
        0
    }
}


#[derive(Debug)]
enum AsmLine<'a> {
    Label(String),
    Instruction(AsmInstruction<'a>),
    Inline(String),
    Comment(String)
}

impl<'a> AsmLine<'a> {
    fn write(&self, compiler_state: &'a CompilerState<'a>, writer: &'a mut dyn Write) -> Result<usize, std::io::Error> {
        let mut s = 0;
        match self {
            AsmLine::Label(string) => { 
                s += writer.write(string.as_bytes())?;
                s += writer.write("\n".as_bytes())?;
            },
            AsmLine::Instruction(inst) => {
                s += writer.write(format!("\t{:23}\t; {} cycles\n", inst.to_dasm(compiler_state), inst.cycles(compiler_state)).as_bytes())?;
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

pub struct Assembly<'a> {
    compiler_state: &'a CompilerState<'a>,
    code: Vec<AsmLine<'a>>
}

impl<'a> Assembly<'a> {
    fn new(compiler_state: &'a CompilerState<'a>) -> Assembly<'a> {
        Assembly {
            compiler_state,
            code: Vec::<AsmLine<'a>>::new()
        }
    }
    fn write(&self, writer: &'a mut dyn Write) -> Result<usize, std::io::Error> {
        let mut s = 0;
        for i in &self.code {
            s += i.write(self.compiler_state, writer)?;
        }
        Ok(s)
    } 
}
