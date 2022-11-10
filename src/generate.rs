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
    for v in state.sorted_variables().iter() {
        file.write_all(format!("{:23}\tds {}\n", v.0, v.1.size).as_bytes())?; 
    }
    Ok(())
}
