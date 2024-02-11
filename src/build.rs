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

use std::io::Write;
use log::debug;

use cc6502::error::Error;
use cc6502::compile::*;
use cc6502::assemble::AssemblyCode;
use cc6502::generate::*;
use cc6502::Args;
use std::collections::{HashSet, HashMap};

fn compute_function_level(function_name: &String, node: &str, current_level: usize, tree: &HashMap<String, Vec<String>>, already_seen: &mut HashSet<String>) -> Option<usize> 
{
    let mut ret = None;
    if let Some(calls) = tree.get(node) {
        // Is the function name in current_level tree ?
        if calls.contains(function_name) {
            ret = Some(current_level);
        }
        // Look at the lower levels if it is possible to find it also
        for nodex in calls {
            //debug!("Function name: {}, {:?}", function_name, nodex);
            if already_seen.contains(nodex) {
                //debug!("Function {} has already been seen", nodex);
                return None;
            }
            already_seen.insert(nodex.clone());
            if let Some(lx) = compute_function_level(function_name, nodex, current_level + 1, tree, already_seen) {
                if let Some(lxx) = ret {
                    if lx > lxx {
                        ret = Some(lx);
                    }
                } else {
                    ret = Some(lx)
                }
            }
            already_seen.remove(nodex);
        }
    } 
    ret 
}

pub fn build_cartridge(compiler_state: &CompilerState, writer: &mut dyn Write, args: &Args) -> Result<(), Error> 
{
    let mut superchip = false;
    let mut bankswitching_scheme = "4K";
    let mut banked_functions = HashSet::<String>::new(); 
    // Try to figure out what is the bankswitching method

    // Let's identitfy superchip
    for v in compiler_state.sorted_variables().iter() {
        if v.1.memory == VariableMemory::Superchip && v.1.def == VariableDefinition::None {
            superchip = true;
        }
    }

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
    } else if compiler_state.context.get_macro("__DPCPLUS__").is_some() {
        bankswitching_scheme = "DPC+";
        if maxbank > 5 {
            return Err(Error::Configuration { error: "DPC+ framework only works with 32KB ROM".to_string() });
        } else {
            maxbank = 5;
        }
    } else if compiler_state.context.get_macro("__3E__").is_some() {
        bankswitching_scheme = "3E";
        maxbank = ((maxbank / 8) + 1) * 8 - 1;
    } else if compiler_state.context.get_macro("__3E_PLUS__").is_some() {
        bankswitching_scheme = "3EP";
        maxbank = ((maxbank / 4) + 1) * 4 - 1;
    }

    let bankswitching_address: u32;
    if bankswitching_scheme == "DPC+" {
        bankswitching_address = 0x1FF6;
    } else if bankswitching_scheme != "3E" && bankswitching_scheme != "3EP" {
        if maxbank > 0 {
            bankswitching_address = match maxbank {
                1 => {
                    if bankswitching_scheme == "4K" {
                        bankswitching_scheme = if superchip {"F8S"} else {"F8"};
                    }
                    0x1FF8
                },
                2 | 3 => {
                    if bankswitching_scheme == "4K" {
                        bankswitching_scheme = if superchip {"F6S"} else {"F6"};
                    }
                    maxbank = 3;
                    0x1FF6
                },
                4 | 5 | 6 | 7 => {
                    if bankswitching_scheme == "4K" {
                        bankswitching_scheme = if superchip {"F4S"} else {"F4"};
                    }
                    maxbank = 7;
                    0x1FF4
                },
                _ => { return Err(Error::Unimplemented { feature: "Bankswitching scheme not implemented" }); },
            };
        } else {
            bankswitching_address = 0;
        }
    } else {
        bankswitching_address = 0;
    }
    
    // Start generation
    let mut gstate = GeneratorState::new(compiler_state, writer, args.insert_code, args.warnings.clone(), bankswitching_scheme);
    gstate.write("\tPROCESSOR 6502\n\n")?;
    
    for v in compiler_state.sorted_variables().iter() {
        if v.1.var_const  {
            if let VariableDefinition::Value(vx) = &v.1.def  {
                match vx {
                    VariableValue::Int(val) => gstate.write(&format!("{:23}\tEQU ${:x}\n", v.0, val)),
                    VariableValue::LowPtr((s, offset)) => if *offset != 0 {
                        gstate.write(&format!("{:23}\tEQU <({} + {})\n", v.0, s, offset))
                    } else {
                        gstate.write(&format!("{:23}\tEQU <{}\n", v.0, s))
                    },
                    VariableValue::HiPtr((s, offset)) => if *offset != 0 {
                        gstate.write(&format!("{:23}\tEQU >({} + {})\n", v.0, s, offset))
                    } else {
                        gstate.write(&format!("{:23}\tEQU >{}\n", v.0, s))
                    },
                }?;
            }
        }
    }
    
    let mut banked_function_address = 0;
    
    for f in compiler_state.sorted_functions().iter() {
        if f.1.code.is_some() {
            gstate.current_bank = f.1.bank;
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

    gstate.compute_functions_actually_in_use()?;

    for f in compiler_state.sorted_functions().iter() {
        if f.1.code.is_some() {
            if f.1.bank == 0 {
                // Compute banked functions
                if let Some(called_functions) = gstate.functions_call_tree.get(f.0) {
                    for i in called_functions {
                        let fx = compiler_state.functions.get(i).unwrap();
                        if fx.bank != 0 {
                            banked_functions.insert(i.clone());
                        }
                    }
                }
            }
        }
    }

    gstate.write("\n\tSEG.U VARS\n\tORG $80\n\n")?;
    
    let mut zeropage_bytes = 1;
    
    // Generate variables code
    gstate.write("cctmp                  \tds 1\n")?; 
    for v in compiler_state.sorted_variables().iter() {
        if v.1.memory == VariableMemory::Zeropage && v.1.def == VariableDefinition::None && v.1.global {
            if v.1.size > 1 {
                let s = match v.1.var_type {
                    VariableType::CharPtr => 1,
                    VariableType::CharPtrPtr => 2,
                    VariableType::ShortPtr => 2,
                    _ => unreachable!()
                };
                gstate.write(&format!("{:23}\tds {}\n", v.0, v.1.size * s))?; 
                zeropage_bytes += v.1.size * s;
            } else {
                let s = match v.1.var_type {
                    VariableType::Char => 1,
                    VariableType::Short => 2,
                    VariableType::CharPtr => 2,
                    VariableType::CharPtrPtr => 2,
                    VariableType::ShortPtr => 2,
                };
                gstate.write(&format!("{:23}\tds {}\n", v.0, s))?; 
                zeropage_bytes += s;
            }
        }
    }

    // Compute in the call tree the level of each function
    let mut function_levels: Vec<Vec<String>> = Vec::new();
    for f in compiler_state.sorted_functions().iter() {
        let lev = if f.0 == "main" { Some(0) } else {
            let mut already_seen = HashSet::new();
            compute_function_level(f.0, "main", 1, &gstate.functions_call_tree, &mut already_seen)
        };
        if let Some(level) = lev {
            let l = function_levels.get_mut(level);
            if let Some(a) = l {
                a.push(f.0.clone())
            } else {
                function_levels.resize(level + 1, Vec::new());
                function_levels[level].push(f.0.clone());
            }
        }
    }
    
    let mut level = 0;
    for l in function_levels {
        let mut maxbsize = 0;
        let mut bsize = 0;
        let mut ft = true;
        for fx in l {
            if let Some(f) = compiler_state.functions.get(&fx) {
                if gstate.functions_actually_in_use.get(&fx).is_some() && f.local_variables.len() != 0 {
                    if ft {
                        gstate.write(&format!("\nLOCAL_VARIABLES_{}\n\n", level))?;
                        ft = false;
                    }
                    bsize = 0;
                    gstate.write(&format!("\tORG LOCAL_VARIABLES_{}\n", level))?;
                    for vx in &f.local_variables {
                        if let Some(v) = compiler_state.variables.get(vx) {
                            if v.memory == VariableMemory::Zeropage && v.def == VariableDefinition::None {
                                if v.size > 1 {
                                    let s = match v.var_type {
                                        VariableType::CharPtr => 1,
                                        VariableType::CharPtrPtr => 2,
                                        VariableType::ShortPtr => 2,
                                        _ => unreachable!()
                                    };
                                    gstate.write(&format!("{:23}\tds {}\n", vx, v.size * s))?; 
                                    bsize += v.size * s;
                                } else {
                                    let s = match v.var_type {
                                        VariableType::Char => 1,
                                        VariableType::Short => 2,
                                        VariableType::CharPtr => 2,
                                        VariableType::CharPtrPtr => 2,
                                        VariableType::ShortPtr => 2,
                                    };
                                    gstate.write(&format!("{:23}\tds {}\n", vx, s))?; 
                                    bsize += s;
                                }
                            }
                        }
                    }
                    if bsize > maxbsize {
                        maxbsize = bsize;
                    }
                }
            }
        }
        if maxbsize != bsize {
            gstate.write(&format!("\tORG LOCAL_VARIABLES_{} + {}\n", level, maxbsize))?;
        }
        zeropage_bytes += maxbsize;
        level += 1;
    }
    if args.verbose {
        println!("Atari 2600 zeropage RAM usage: {}/128", zeropage_bytes);
    }
    if zeropage_bytes > 128 {
        return Err(Error::Configuration { error: "Memory full. Zeropage Atari 2600 RAM is limited to 128 bytes".to_string() });
    }

    if superchip {
        if args.verbose {
            println!("Superchip RAM : 0x1000 onwards");
        }
        let mut filled = 0;
        gstate.write("\n\tSEG.U SUPERVARS\n\tORG $1000\n\tRORG $1000\n")?;
        // Superchip variables
        for v in compiler_state.sorted_variables().iter() {
            if v.1.memory == VariableMemory::Superchip && v.1.def == VariableDefinition::None {
                let sx = if v.1.size > 1 {
                    let s = match v.1.var_type {
                        VariableType::CharPtr => 1,
                        VariableType::CharPtrPtr => 2,
                        VariableType::ShortPtr => 2,
                        _ => unreachable!()
                    };
                    v.1.size * s 
                } else {
                    match v.1.var_type {
                        VariableType::Char => 1,
                        VariableType::Short => 2,
                        VariableType::CharPtr => 2,
                        VariableType::CharPtrPtr => 2,
                        VariableType::ShortPtr => 2,
                    }
                };
                filled += sx;
                if filled > 128 {
                    return Err(Error::Configuration { error: "Memory full. Superchip RAM is limited to 128 bytes".to_string() });
                }
                gstate.write(&format!("{:23}\tds {}\n", v.0, sx))?;
                if args.verbose {
                    println!(" - {} ({} byte{})", v.0, sx, if sx > 1 {"s"} else {""});
                }
            }
        }
        if args.verbose {
            println!("Superchip RAM usage: {}/128", filled);
        }
    }

    // Generate RAM for 3E bankswitching scheme
    if bankswitching_scheme == "3E" {
        for bank in 1..=512 { // Max 512ko
            let mut first = true;
            let mut filled = 0;
            for v in compiler_state.sorted_variables().iter() {
                if v.1.memory == VariableMemory::MemoryOnChip(bank) && v.1.def == VariableDefinition::None {
                    if first {
                        first = false;
                        if args.verbose {
                            println!("Bank #{bank} - 3E RAM : 0x1000 onwards");
                        }
                        gstate.write(&format!("\n\tSEG.U RAM_3E_{}\n\tORG $1000\n\tRORG $1000\n", bank))?;
                    }
                    let sx = if v.1.size > 1 {
                        let s = match v.1.var_type {
                            VariableType::CharPtr => 1,
                            VariableType::CharPtrPtr => 2,
                            VariableType::ShortPtr => 2,
                            _ => unreachable!()
                        };
                        v.1.size * s 
                    } else {
                        match v.1.var_type {
                            VariableType::Char => 1,
                            VariableType::Short => 2,
                            VariableType::CharPtr => 2,
                            VariableType::CharPtrPtr => 2,
                            VariableType::ShortPtr => 2,
                        }
                    };
                    filled += sx;
                    if filled > 1024 {
                        return Err(Error::Configuration { error: "Memory full. 3E RAM is limited to 1024 bytes per bank".to_string() });
                    }
                    gstate.write(&format!("{:23}\tds {}\n", v.0, sx))?;
                    if args.verbose {
                        println!(" - {} ({} byte{})", v.0, sx, if sx > 1 {"s"} else {""});
                    }
                }
            }
        }
    }

    // Generate RAM for 3E+ bankswitching scheme
    if bankswitching_scheme == "3EP" {
        for bank in 0..64 { // Max 32ko
            let mut first = true;
            let mut filled = 0;
            for v in compiler_state.sorted_variables().iter() {
                if v.1.memory == VariableMemory::MemoryOnChip(bank) && v.1.def == VariableDefinition::None {
                    if first {
                        first = false;
                        let segment = 3 - (bank & 3);
                        let address = 0x1000 + segment * 0x400;
                        gstate.write(&format!("\n\tSEG.U RAM_3E_{}\n\tORG ${:04x}\n\tRORG ${:04x}\n", bank, address, address))?;
                        if args.verbose {
                            println!("Bank #{bank} - 3E+ RAM : 0x{:04x} onwards", address);
                        }
                    }
                    let sx = if v.1.size > 1 {
                        let s = match v.1.var_type {
                            VariableType::CharPtr => 1,
                            VariableType::CharPtrPtr => 2,
                            VariableType::ShortPtr => 2,
                            _ => unreachable!()
                        };
                        v.1.size * s 
                    } else {
                        match v.1.var_type {
                            VariableType::Char => 1,
                            VariableType::Short => 2,
                            VariableType::CharPtr => 2,
                            VariableType::CharPtrPtr => 2,
                            VariableType::ShortPtr => 2,
                        }
                    };
                    filled += sx;
                    if filled > 512 {
                        return Err(Error::Configuration { error: "Memory full. 3E+ RAM is limited to 512 bytes per bank".to_string() });
                    }
                    gstate.write(&format!("{:23}\tds {}\n", v.0, sx))?;
                    if args.verbose {
                        println!(" - {} ({} byte{})", v.0, sx, if sx > 1 {"s"} else {""});
                    }
                }
            }
        }
    }

    if maxbank > 0 && bankswitching_scheme != "3E" && bankswitching_scheme != "3EP" {
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
    }

    // Generate functions code
    gstate.write("\n; Functions definitions\n\tSEG CODE\n")?;

    // Generate code for all banks
    for b in 0..=maxbank {

        let mut filled = 0;
        let (bank, banksize, rorg) = if bankswitching_scheme == "3E" { 
            if b == maxbank { (0, 0x0800, 0x1800) } else { (b + 1, 0x0800, 0x1000) }
        } else if bankswitching_scheme == "3EP" { 
            (b, 0x0400, 0x1000 + (3 - b & 3) * 0x400) 
        } else {(b, 0x1000, 0x1000)};

        // Prelude code for each bank
        debug!("Generating code for bank #{}", bank);
        if args.verbose {
            println!("Bank #{}: Filling memory at ${:04x} (RORG=${:04x})", bank, b * banksize, rorg);
        }
        if args.verbose {
            println!("Bank #{bank}: Generating code at ${:04x}", rorg);
        }
        gstate.current_bank = bank;
        gstate.write(&format!("\n\tORG ${:04x}\n\tRORG ${:04x}\n", b * banksize, rorg))?;
   
        if superchip {
            gstate.write("\n\tDS 256, $FF\n")?;
            filled = 256;
            if args.verbose {
                println!(" - Superchip RAM data : 256/{banksize}");
            }
        } else if bankswitching_scheme == "DPC" || bankswitching_scheme == "DPC+" {
            gstate.write("\n\tDS 128, $00\n")?;
            filled = 128;
            if args.verbose {
                println!(" - DPC data : 128/{banksize}");
            }
        }

        if maxbank > 0 && bankswitching_scheme != "3E" && bankswitching_scheme != "3EP" {
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
            filled += 9 + 7;
            if args.verbose {
                println!(" - Trampoline code : {filled}/{banksize}");
            }
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
        CPX #$40	
        BNE .loop
        ")?;

            filled += 14;

            if bankswitching_scheme == "3EP" {
                gstate.write("
        LDA #$81 ; ROM Bank 1 to segment 2
        STA ROM_SELECT
        LDA #$42 ; ROM Bank 2 to segment 1
        STA ROM_SELECT
        LDA #$03 ; ROM Bank 3 to segment 0
        STA ROM_SELECT
        ")?;
                filled += 12;

            }
            
            gstate.write("
        JMP main
        ")?;
            filled += 3;
            if args.verbose {
                println!(" - Powerup code : {filled}/{banksize}");
            }
        }
        
        // Generate included assembler
        for (i, asm) in (&compiler_state.included_assembler).iter().enumerate() {
            let basm = if let Some(b) = asm.3 {
                b as u32
            } else { 0 };
            debug!("assembler: {} {} {}", i, bank, basm);
            if bank == basm {
                gstate.write(&asm.0)?;
                let name;
                if let Some(n) = &asm.1 {
                    name = n.as_str();
                } else {
                    name = "Unknown";
                }
                if let Some(s) = asm.2 {
                    filled += s as u32;
                    if args.verbose {
                        println!(" - Assembler {} code (filled {}/{})", name, filled, banksize);
                    }
                } else {
                    let nl = asm.0.lines().count() as u32; 
                    filled += nl * 3; // 3 bytes default per line estimate.
                    if args.verbose {
                        println!(" - Assembler {} code (filled {}/{} - estimated)", name, filled, banksize);
                    }
                }
            }
        }

        // Generate functions code
        for f in compiler_state.sorted_functions().iter() {
            if f.1.code.is_some() && !f.1.inline && f.1.bank == bank && gstate.functions_actually_in_use.get(f.0).is_some() {
                debug!("Generating code for function {}", f.0);

                gstate.write(&format!("\n{}\tSUBROUTINE\n", f.0))?;
                gstate.write_function(f.0)?;
                gstate.write("\tRTS\n")?;
       
                if args.verbose {
                    let s = gstate.functions_code.get(f.0).unwrap().size_bytes() + 1;
                    filled += s;
                    println!(" - {} function (filled {}/{})", f.0, filled, banksize);
                }
            }
        }

        // Generate ROM tables
        gstate.write("\n; Tables in ROM\n")?;
        if args.verbose {
            println!("Bank #{bank}: Inserting ROM tables");
        }
            
        for v in compiler_state.sorted_variables().iter() {
            if let VariableMemory::ROM(rom_bank) = v.1.memory {
                if rom_bank == bank {
                    let s = if filled > 0 {
                        (((filled - 1) / v.1.alignment as u32) + 1) * v.1.alignment as u32
                    } else { 0 };
                    let s2 = match &v.1.def {
                        VariableDefinition::Array(arr) => {
                            if v.1.alignment != 1 {
                                gstate.write(&format!("\n\talign {}\n", v.1.alignment))?;
                            }
                            gstate.write(v.0)?;
                            let mut counter = 0;
                            for vx in arr {
                                match vx {
                                    VariableValue::Int(i) => {
                                        if counter == 0 {
                                            gstate.write("\n\thex ")?;
                                        }
                                        counter += 1;
                                        if counter == 16 { counter = 0; }
                                        gstate.write(&format!("{:02x}", i & 0xff))
                                    },
                                    VariableValue::LowPtr((s, offset)) => {
                                        counter = 0;
                                        if *offset != 0 {
                                            gstate.write(&format!("\n\t.byte <({} + {})", s, offset))
                                        } else {
                                            gstate.write(&format!("\n\t.byte <{}", s))
                                        }
                                    },
                                    VariableValue::HiPtr((s, offset)) => {
                                        counter = 0;
                                        if *offset != 0 {
                                            gstate.write(&format!("\n\t.byte >({} + {})", s, offset))
                                        } else {
                                            gstate.write(&format!("\n\t.byte >{}", s))
                                        }
                                    },
                                }?;
                            } 
                            if v.1.var_type == VariableType::ShortPtr {
                                for vx in arr {
                                    if counter == 0 {
                                        gstate.write("\n\thex ")?;
                                    }
                                    counter += 1;
                                    if counter == 16 { counter = 0; }
                                    if let VariableValue::Int(i) = vx {
                                        gstate.write(&format!("{:02x}", (i >> 8) & 0xff))?;
                                    }
                                } 
                            }
                            gstate.write("\n")?;
                            if v.1.var_type == VariableType::ShortPtr {
                                arr.len() * 2
                            } else {
                                arr.len()
                            }
                        },
                        VariableDefinition::ArrayOfPointers(arr) => {
                            if v.1.alignment != 1 {
                                gstate.write(&format!("\n\talign {}\n", v.1.alignment))?;
                            }
                            gstate.write(v.0)?;

                            let mut counter = 0;
                            for i in arr {
                                if counter % 8 == 0 {
                                    gstate.write("\n\t.byte ")?;
                                }
                                counter += 1;
                                if i.0 == "__address__" {
                                    gstate.write(&format!("${:02x}", i.1 & 0xff))?;
                                } else {
                                    if i.1 != 0 {
                                        gstate.write(&format!("<({} + {})", i.0, i.1))?;
                                    } else {
                                        gstate.write(&format!("<{}", i.0))?;
                                    }
                                }
                                if counter % 8 != 0 {
                                    gstate.write(", ")?;
                                } 
                            } 
                            for i in arr {
                                if counter % 8 == 0 {
                                    gstate.write("\n\t.byte ")?;
                                }
                                counter += 1;
                                if i.0 == "__address__" {
                                    gstate.write(&format!("${:02x}", i.1 >> 8))?;
                                } else {
                                    if i.1 != 0 {
                                        gstate.write(&format!(">({} + {})", i.0, i.1))?;
                                    } else {
                                        gstate.write(&format!(">{}", i.0))?;
                                    }
                                }
                                if counter % 8 != 0 && counter < 2 * arr.len() {
                                    gstate.write(", ")?;
                                }
                            } 
                            gstate.write("\n")?;
                            arr.len() * 2
                        },
                        _ => 0 
                    } as u32;
                    filled = s + s2;
                    if args.verbose {
                        println!(" - {} array (filled {}/{})", v.0, filled, banksize);
                    }
                }
            }
        }
            
        // Epilogue code
        if bankswitching_scheme == "3E" {
            if bank == 0 {
                gstate.write("
        ECHO ([$1FF0-.]d), \"bytes free in bank 0\"
        ")?;
            } else {
                gstate.write(&format!("
        ECHO ([$1800-.]d), \"bytes free in bank {}\"
        ", bank))?;
            }
        } else if bankswitching_scheme == "3EP" {
            if bank == 0 {
                gstate.write("
        ECHO ([$1FF0-.]d), \"bytes free in bank 0\"
        ")?;
            } else {
                gstate.write(&format!("
        ECHO ([${:04x}-.]d), \"bytes free in bank {}\"
        ", rorg + 0x400, bank))?;
            }
        }
        else {
            let end_of_memory = if banked_functions.len() != 0 {
                0x1fef - banked_functions.len() * 10
            } else {
                if compiler_state.variables.get("PLUSROM_API").is_some() {
                    0x1fef
                } else {
                    0x1ffa
                }
            };
            gstate.write(&format!("
        ECHO ([${:04x}-.]d), \"bytes free in bank {}\"
        ", end_of_memory, bank))?;

            if bank == 0 {
                if banked_functions.len() > 0 {
                    // Generate bankswitching functions code
                    banked_function_address = 0x0FEF - banked_functions.len() * 10;
                    debug!("Banked function address={:04x}", banked_function_address);
                    gstate.write(&format!("
        ORG ${:04x}
        RORG ${:04x}", 
        banked_function_address, 0x1000 + banked_function_address))?;
                    for bank_ex in 1..=maxbank {
                        for f in compiler_state.sorted_functions().iter() {
                            if f.1.code.is_some() && !f.1.inline && f.1.bank == bank_ex && banked_functions.contains(f.0) {
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
                }
            } else {
                for f in compiler_state.sorted_functions().iter() {
                    let address = banked_function_address;
                    if f.1.code.is_some() && !f.1.inline && f.1.bank == bank && banked_functions.contains(f.0) {
                        debug!("#{} Banked function address={:04x}", bank, banked_function_address);
                        gstate.write(&format!("
        ORG ${:04x}
        RORG ${:04x}
        JSR {}
        LDX ${:04x}
                    ", address + f.1.bank as usize * 0x1000 + 3, 0x1000 + address + 3, f.0, bankswitching_address))?;
                    banked_function_address += 10;
                    }
                }
            }
        }

        let starting_code = if maxbank > 0 && bank != 0 { "Start" } else { "Powerup" };

        if b == maxbank && compiler_state.variables.get("PLUSROM_API").is_some() {
            let v = compiler_state.get_variable("PLUSROM_API");
            let offset = match v.memory {
                VariableMemory::ROM(bank) => bank,
                _ => 0,
            };
            gstate.write(&format!("
        ORG ${}FFA
        RORG $1FFA

        .word PLUSROM_API + ${:04x}\t
        .word {}\t; RESET
        .word {}\t; IRQ
        \n", bank, offset * 0x1000, starting_code, starting_code))?;
        } else if bankswitching_scheme == "3EP" {
            if bank == 0 {
                gstate.write("
        ORG $03FA
        RORG $1FFA

        .word Powerup\t; NMI
        .word Powerup\t; RESET
        .word Powerup\t; IRQ
        \n")?; 
            } else if bank == maxbank {
                gstate.write(&format!("
            ORG ${:04x} 
            DS 1, 0x81
            ", bank * 0x400 + 0x3ff))?;
            }
        } else if bankswitching_scheme != "DPC+" && bankswitching_scheme != "3E" {
            gstate.write(&format!("
        ORG ${}FFA
        RORG $1FFA

        .word {}\t; NMI
        .word {}\t; RESET
        .word {}\t; IRQ
        \n", 
        bank, starting_code, starting_code, starting_code))?;
        } else if b == maxbank {
            gstate.write(&format!("
        ORG ${:04x}
        RORG $1FFA

        .word {}\t; NMI
        .word {}\t; RESET
        .word {}\t; IRQ
        \n", 
        (b + 1) * banksize - 6, starting_code, starting_code, starting_code))?;
        }
    }

    if bankswitching_scheme == "DPC" {
        gstate.write("
            SEG DISPLAY
            ORG $2000
            RORG $0000
            ")?;
        
        // Generate display tables
        gstate.write("\n; Display in ROM\n")?;
        for v in compiler_state.sorted_variables().iter() {
            if let VariableMemory::Display = v.1.memory {
                if let VariableDefinition::Array(arr) = &v.1.def {
                    if v.1.alignment != 1 {
                        gstate.write(&format!("\n\talign {}\n", v.1.alignment))?;
                    }
                    gstate.write(v.0)?;
                    let mut counter = 0;
                    for vx in arr {
                        if counter == 0 || counter == 16 {
                            gstate.write("\n\thex ")?;
                        }
                        counter += 1;
                        if counter == 16 { counter = 0; }
                        if let VariableValue::Int(i) = vx {
                            gstate.write(&format!("{:02x}", i & 0xff))?;
                        }
                    } 
                    gstate.write("\n")?;
                }
            }
        }
        gstate.write("
            ECHO ([$800-.]d), \"bytes free in DPC display memory\"

            ORG $27FF
            DS 1, 0x81
            ")?;
    }
 
    if bankswitching_scheme == "DPC+" {
        gstate.write("
            SEG DISPLAY
            ORG $6000
            RORG $0000
            ")?;
        
        // Generate display tables
        gstate.write("\n; Display in RAM\n")?;
        for v in compiler_state.sorted_variables().iter() {
            if let VariableMemory::Display = v.1.memory {
                if v.1.alignment != 1 {
                    gstate.write(&format!("\n\talign {}\n", v.1.alignment))?;
                }
                if let VariableDefinition::Array(arr) = &v.1.def {
                    gstate.write(v.0)?;
                    let mut counter = 0;
                    for vx in arr {
                        if counter == 0 || counter == 16 {
                            gstate.write("\n\thex ")?;
                        }
                        counter += 1;
                        if counter == 16 { counter = 0; }
                        if let VariableValue::Int(i) = vx {
                            gstate.write(&format!("{:02x}", i & 0xff))?;
                        }
                    } 
                    gstate.write("\n")?;
                } else {
                    if v.1.size > 1 {
                        let s = match v.1.var_type {
                            VariableType::CharPtr => 1,
                            VariableType::CharPtrPtr => 2,
                            VariableType::ShortPtr => 2,
                            _ => unreachable!()
                        };
                        gstate.write(&format!("{:23}\tds {}\n", v.0, v.1.size * s))?; 
                    } else {
                        let s = match v.1.var_type {
                            VariableType::Char => 1,
                            VariableType::Short => 2,
                            VariableType::CharPtr => 2,
                            VariableType::CharPtrPtr => 2,
                            VariableType::ShortPtr => 2,
                        };
                        gstate.write(&format!("{:23}\tds {}\n", v.0, s))?; 
                    }
                }
            }
        }
        gstate.write("
            ECHO ([$1000-.]d), \"bytes free in DPC+ display memory\"
            ")?;
        
        gstate.write("
            SEG FREQUENCIES
            ORG $7000
            RORG $0000
            ")?;
        
        // Generate display tables
        gstate.write("\n; Frequencies in ROM\n")?;
        for v in compiler_state.sorted_variables().iter() {
            if let VariableMemory::Frequency = v.1.memory {
                if let  VariableDefinition::Array(arr) = &v.1.def {
                    if v.1.alignment != 1 {
                        gstate.write(&format!("\n\talign {}\n", v.1.alignment))?;
                    }
                    gstate.write(v.0)?;
                    let mut counter = 0;
                    for vx in arr {
                        if counter == 0 || counter == 16 {
                            gstate.write("\n\thex ")?;
                        }
                        counter += 1;
                        if counter == 16 { counter = 0; }
                        if let VariableValue::Int(i) = vx {
                            gstate.write(&format!("{:02x}", i & 0xff))?;
                        }
                    } 
                    gstate.write("\n")?;
                }
            }
        }
        gstate.write("
            ECHO ([$400-.]d), \"bytes free in DPC+ frequency memory\"

            ORG $73FF
            DS 1, 0x81
            ")?;
    }
    gstate.write("\tEND\n")?;
    
    if args.verbose {
        println!("Generated a {} ATARI 2600 cartridge", bankswitching_scheme);
    }
    Ok(())
}
