use std::io::Write;
use log::debug;

use crate::error::Error;
use crate::compile::*;
use crate::assemble::AssemblyCode;
use crate::generate::*;
use crate::Args;

pub fn build_cartridge(compiler_state: &CompilerState, writer: &mut dyn Write, args: &Args) -> Result<(), Error> 
{
    let mut gstate = GeneratorState::new(compiler_state, writer, args.insert_code);
    let mut superchip = false;
    let mut bankswitching_scheme = "Unknown";

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
       // debug!("{:?}",v);
        if !v.1.var_const && v.1.memory == VariableMemory::Zeropage && v.1.def == VariableDefinition::None {
            let s = match v.1.var_type {
                VariableType::Char => 1,
                VariableType::Short => 2,
                VariableType::CharPtr => 2,
            };
            if v.1.size > 1 {
                gstate.write(&format!("{:23}\tds {}\n", v.0, v.1.size))?; 
            } else {
                gstate.write(&format!("{:23}\tds {}\n", v.0, s))?; 
            }
        }
        if !v.1.var_const && v.1.memory == VariableMemory::Superchip && v.1.def == VariableDefinition::None {
            superchip = true;
        }
    }

    if superchip {
        gstate.write("\n\tSEG.U SUPERVARS\n\tORG $1000\n\tRORG $1000\n")?;
        // Superchip variables
        for v in compiler_state.sorted_variables().iter() {
            if !v.1.var_const && v.1.memory == VariableMemory::Superchip && v.1.def == VariableDefinition::None {
                let s = match v.1.var_type {
                    VariableType::Char => 1,
                    VariableType::Short => 2,
                    VariableType::CharPtr => 2,
                };
                gstate.write(&format!("{:23}\tds {}\n", v.0, v.1.size * s))?; 
            }
        }
    }

    // Try to figure out what is the bankswitching method
    let mut maxbank = 0;
    for f in compiler_state.sorted_functions().iter() {
         if f.1.bank > maxbank { maxbank = f.1.bank; }
    }
    // Minimum 8K for superchip
    if superchip && maxbank == 0 {
        maxbank = 1;
    }
    // Are we producing a DPC cartridge ?
    if compiler_state.context.get_macro("__DPC__").is_some() {
        bankswitching_scheme = "DPC";
        if maxbank > 1 {
            return Err(Error::Configuration { error: "DPC chip only works with 8KB ROM".to_string() });
        } else {
            maxbank = 1;
        }
    }

    let bankswitching_address: u32;
    if maxbank > 0 {
        bankswitching_address = match maxbank {
            1 => {
                if bankswitching_scheme == "Unknown" {
                    bankswitching_scheme = if superchip {"F8S"} else {"F8"};
                }
                0x1FF8
            },
            2 | 3 => {
                if bankswitching_scheme == "Unknown" {
                    bankswitching_scheme = if superchip {"F6S"} else {"F6"};
                }
                maxbank = 3;
                0x1FF6
            },
            4 | 5 | 6 | 7 => {
                if bankswitching_scheme == "Unknown" {
                    bankswitching_scheme = if superchip {"F4S"} else {"F4"};
                }
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
    
    for f in compiler_state.sorted_functions().iter() {
        if f.1.code.is_some() {
            if f.1.bank != 0 {
                nb_banked_functions += 1;
            }

            gstate.local_label_counter_for = 0;
            gstate.local_label_counter_if = 0;

            gstate.functions_code.insert(f.0.clone(), AssemblyCode::new());
            gstate.current_function = Some(f.0.clone());
            gstate.generate_statement(f.1.code.as_ref().unwrap())?;
            gstate.current_function = None;

            if args.optimization_level > 0 {
                gstate.optimize_function(f.0);
            }
            gstate.check_branches(f.0);
        }
     }

    for bank in 0..=maxbank {
        // Prelude code for each bank
        debug!("Generating code for bank #{}", bank);
        gstate.current_bank = bank;
        gstate.write(&format!("\n\tORG ${:04x}\n\tRORG $1000\n", bank * 0x1000))?;
   
        if superchip {
            gstate.write("\n\tDS 256, $FF\n")?;
        } else if bankswitching_scheme == "DPC" {
            gstate.write("\n\tDS 128, $00\n")?;
        }

        if maxbank > 0 {
            // Generate trampoline code
            gstate.write("
;----The following code is the same on all banks----
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

            // Generate included assembler
            for asm in &compiler_state.included_assembler {
                gstate.write(&asm)?;
            }
        }
        
        // Generate functions code
        for f in compiler_state.sorted_functions().iter() {
            if f.1.code.is_some() && !f.1.inline && f.1.bank == bank {
                debug!("Generating code for function #{}", f.0);

                gstate.write(&format!("\n{}\tSUBROUTINE\n", f.0))?;
                gstate.write_function(f.0)?;
                gstate.write("\tRTS\n")?;
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
        ECHO ([${:04x}-.]d), \"bytes free in bank {}\"
        ", 0x1FF0 - nb_banked_functions * 10, bank))?;

        if bank == 0 {
            // Generate bankswitching functions code
            banked_function_address = 0x0FF0 - nb_banked_functions * 10;
            debug!("Banked function address={:04x}", banked_function_address);
            gstate.write(&format!("
        ORG ${:04x}
        RORG ${:04x}", banked_function_address, 0x1000 + banked_function_address))?;
            for bank_ex in 1..=maxbank {
                for f in compiler_state.sorted_functions().iter() {
                    if f.1.code.is_some() && !f.1.inline && f.1.bank == bank_ex {
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
                if f.1.code.is_some() && !f.1.inline && f.1.bank == bank {
                    debug!("#{} Banked function address={:04x}", bank, banked_function_address);
                    gstate.write(&format!("
        ORG ${:04x}
        RORG ${:04x}
        JSR {}
        LDX ${:04x}
                    ", address + f.1.bank * 0x1000 + 3, 0x1000 + address + 3, f.0, bankswitching_address))?;
                    banked_function_address += 10;
                }
            }
        }

        let starting_code = if maxbank > 0 && bank != 0 { "Start" } else { "Powerup" };

        if bank == maxbank && compiler_state.variables.get("PLUSROM_API").is_some() {
            let v = compiler_state.get_variable("PLUSROM_API");
            let offset = match v.memory {
                VariableMemory::ROM(bank) => bank,
                _ => 0,
            };
            gstate.write(&format!("
        ORG ${}FFA
        RORG $1FFA

        .word PLUSROM_API + ${:04x}\t
        .word PLUSROM_API + ${:04x}\t
        .word {}\t; IRQ
        \n", bank, offset * 0x1000, offset * 0x1000, starting_code))?;
        } else {
            gstate.write(&format!("
        ORG ${}FFA
        RORG $1FFA

        .word {}\t; NMI
        .word {}\t; RESET
        .word {}\t; IRQ
        \n", bank, starting_code, starting_code, starting_code))?;
        }
    }

    if bankswitching_scheme == "DPC" {
        gstate.write(&format!("
            SEG DISPLAY
            ORG $2000
            RORG $0000
            "))?;
        
        // Generate display tables
        gstate.write("\n; Display in ROM\n")?;
        for v in compiler_state.sorted_variables().iter() {
            if let VariableMemory::Display = v.1.memory {
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
        gstate.write(&format!("
            ECHO ([$800-.]d), \"bytes free in DPC display memory\"

            ORG $27FF
            DS 1, 0x81
            "))?;
    }
 
    gstate.write("\tEND\n")?;
    
    if args.verbose {
        println!("Generated a {} ATARI 2600 cartridge", bankswitching_scheme);
    }
    Ok(())
}
