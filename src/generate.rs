use crate::error::Error;
use crate::compile::*;

use std::io::prelude::*;
use std::fs::File;

use log::debug;

struct GeneratorState<'a> {
    last_included_line_number: usize,
    last_included_position: usize,
    last_included_char: std::str::Chars<'a>,
    file: File,
}

enum Hint {
    MoveToA,
    MoveToX,
    MoveToY,
}

fn generate_included_source_code_line<'a>(loc: usize, gstate: &'a mut GeneratorState) -> Option<&'a str>
{
    let mut start_of_line = gstate.last_included_char.clone();
    let mut start_of_line_pos = gstate.last_included_position;
    // Let's find the line including loc
    while gstate.last_included_position < loc {
        let c = gstate.last_included_char.next();
        if c.is_none() { return None; }
        let c = c.unwrap();
        gstate.last_included_position += 1;
        if c == '\n' { 
            gstate.last_included_line_number += 1;
            start_of_line = gstate.last_included_char.clone();
            start_of_line_pos = gstate.last_included_position;
        }
    };
    // Ok, we have found loc. Let's go to the end of line
    loop {
        let c = gstate.last_included_char.next();
        if c.is_none() { return Some(start_of_line.as_str()); }
        let c = c.unwrap();
        gstate.last_included_position += 1;
        if c == '\n' {
            gstate.last_included_line_number += 1;
            return Some(&start_of_line.as_str()[0..(gstate.last_included_position - start_of_line_pos)]);
        }
    }
}

fn generate_assign(lhs: &Expr, rhs: &Expr, state: &State, gstate: &mut GeneratorState, pos: usize) -> Result<(), Error>
{
    match lhs {
        Expr::Var((var, sub)) => {
            match *var {
                "X" => {
                    generate_expr(rhs, state, gstate, pos, Some(Hint::MoveToX))?;
                },
                "Y" => {
                    generate_expr(rhs, state, gstate, pos, Some(Hint::MoveToY))?;
                },
                variable => {
                    generate_expr(rhs, state, gstate, pos, Some(Hint::MoveToA))?;
                    gstate.file.write(format!("\tSTA {}\n", variable).as_bytes())?;
                }
            }
        }
        ,
        _ => return Err(syntax_error(state, "Bad left value in assignement", pos)),
    }
    Ok(())
}

fn generate_expr(expr: &Expr, state: &State, gstate: &mut GeneratorState, pos: usize, hint: Option<Hint>) -> Result<(), Error>
{
    // Include C source code into generated asm
    let included_source_code = generate_included_source_code_line(pos, gstate);
    let line_to_be_written = if let Some(line) = included_source_code {
        Some(line.to_string())
    } else {
        None
    };
    if let Some(l) = line_to_be_written {
        let f = &mut gstate.file; 
        f.write_all(b";")?;
        f.write_all(l.as_bytes())?; // Should include the '\n'
    }

    debug!("Expression: {:?}", expr);
    match expr {
        Expr::Integer(i) => {
            match hint {
                Some(Hint::MoveToA) | None => gstate.file.write(format!("\tLDA #{}\n", i).as_bytes())?,
                Some(Hint::MoveToX) => gstate.file.write(format!("\tLDX #{}\n", i).as_bytes())?,
                Some(Hint::MoveToY) => gstate.file.write(format!("\tLDY #{}\n", i).as_bytes())?,
            };
        },
        Expr::BinOp {lhs, op, rhs} => {
            match op {
                Operation::Assign => {
                    generate_assign(lhs, rhs, state, gstate, pos)?;
                },
                Operation::Add => return Err(Error::Unimplemented { feature: "Addition not implemented" }),
                Operation::Sub => return Err(Error::Unimplemented { feature: "Subtraction not implemented" }),
            };
        },
        _ => unreachable!() 
    }

    Ok(())
}

fn generate_statement(code: &StatementLoc, state: &State, gstate: &mut GeneratorState) -> Result<(), Error>
{
    // Generate different kind of statements
    match &code.statement {
        Statement::Block(statements) => {
            for code in statements {
                generate_statement(&code, state, gstate)?;
            }
        },
        Statement::Expression(expr) => generate_expr(&expr, state, gstate, code.pos, None)?,
    }
    Ok(())
}

pub fn generate_asm(state: &State, filename: &str) -> Result<(), Error> 
{
    let file = File::create(filename)?;

    let mut gstate = GeneratorState {
        last_included_line_number: 0,
        last_included_position: 0,
        last_included_char: state.preprocessed_utf8.chars(),
        file,
    };

    gstate.file.write_all(b"\tPROCESSOR 6502\n")?;
    //gstate.file.write_all(b"\tINCLUDE \"vcs.h\"\n\n")?;
    gstate.file.write_all(b"\tSEG.U variables\n\tORG $80\n\n")?;
    
    // Generate vaiables code
    for v in state.sorted_variables().iter() {
        gstate.file.write_all(format!("{:23}\tds {}\n", v.0, v.1.size).as_bytes())?; 
    }

    // Generate functions code
    gstate.file.write_all("\n; Functions definitions\n\n".as_bytes())?;
    for f in state.sorted_functions().iter() {
        gstate.file.write_all(format!("{}:\n", f.0).as_bytes())?;
        generate_statement(&f.1.code, state, &mut gstate)?;
    }

    Ok(())
}
