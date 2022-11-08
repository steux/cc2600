use crate::error::Error;
use crate::compile::State;

use std::io::prelude::*;
use std::fs::File;

pub fn generate_asm(state: &State, filename: &str) -> Result<(), Error> 
{
    let mut file = File::create(filename)?;
    file.write_all(b"\tPROCESSOR 6502\n")?;
    file.write_all(b"\tINCLUDE \"vcs.h\"\n\n")?;
    file.write_all(b"\tSEG.U variables\n\tORG $80\n\n")?;
    for v in &state.variables {
        file.write_all(format!("{:23}\tds {}\n", v.name, v.size).as_bytes())?; 
    }
    Ok(())
}
