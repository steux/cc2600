use crate::generate::ExprType;

#[derive(Debug)]
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

#[derive(Debug)]
struct AsmInstruction<'a> {
    mnemonic: AsmMnemonic,
    operand: ExprType<'a>,
    high_byte: bool,
}

#[derive(Debug)]
enum AsmLine<'a> {
    Label(String),
    Instruction(AsmInstruction<'a>)
}

#[derive(Debug)]
pub struct Assembly<'a> {
    code: Vec<AsmLine<'a>>
}
