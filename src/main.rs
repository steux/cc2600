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

mod cpp;
mod error;
mod compile;
mod build;
mod generate;
mod assemble;

use std::fs::File;
use std::io::BufReader;
use compile::compile;

extern crate pest;
#[macro_use]
extern crate pest_derive;

use clap::Parser as ClapParser;

#[derive(ClapParser, Debug)]
#[command(author, version, about = "cc2600 - a subset of C compiler for the Atari 2600", long_about = "cc2600 - a subset of C compiler for the Atari 2600\nCopyright (C) 2023 Bruno STEUX\n\nThis program comes with ABSOLUTELY NO WARRANTY;\nThis is free software, and you are welcome to redistribute it\nunder certain conditions;")]
pub struct Args {
    /// Input file name
    input: String,

    /// Preprocessor definitions
    #[arg(short='D')]
    defines: Vec<String>,

    /// Optimization level
    #[arg(short='O', default_value="1", value_parser=clap::value_parser!(u8).range(0..=3))]
    optimization_level: u8,

    /// Verbosity 
    #[arg(short='v', default_value="false")]
    verbose: bool,

    /// Include directories
    #[arg(short='I')]
    include_directories: Vec<String>,

    /// Output file name
    #[arg(short, long, default_value="out.a")]
    output: String,

    /// Insert C code as comments
    #[arg(long, default_value="true")]
    insert_code: bool
}

fn main() {
    env_logger::init();
    let args = Args::parse();

    let f = match File::open(&args.input) {
        Ok(file) => file,
        Err(err) => {
            eprintln!("{}", err);
            std::process::exit(1);
        }
    };
    let reader = BufReader::new(f);
    let mut writer = match File::create(&args.output) {
        Ok(file) => file,
        Err(err) => {
            eprintln!("{}", err);
            std::process::exit(1);
        }
    };

    match compile(reader, &mut writer, &args) {
        Err(e) => {
            eprintln!("{}", e);
            std::process::exit(1) 
        },
        Ok(_) => std::process::exit(0) 
    }
}

#[cfg(test)]
mod tests {
    use super::Args;
    use super::compile::compile;
    use std::str;

    #[test]
    fn for_statement_test1() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1,
        };
        let input = "unsigned char i; void main() { for (i = 0; i < 10; i++); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tCMP #10\n\tBCS .forend1\n.for1\n.forupdate1\n\tINC i\n\tLDA i\n\tCMP #10\n\tBCC .for1\n.forend1\n"));
    }
    
    #[test]
    fn for_statement_test2() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "unsigned char i; void main() { for (i = 0; i != 10; i++); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n.for1\n.forupdate1\n\tINC i\n\tLDA i\n\tCMP #10\n\tBNE .for1\n.forend1\n"));
    }
    
    #[test]
    fn plusplus_statement_test1() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "unsigned char i, j; void main() { i = 0; j = i++; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tSTA j\n\tINC i\n"));
    }
    
    #[test]
    fn plusplus_statement_test2() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "unsigned char i, j; void main() { i = 0; j = ++i; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tINC i\n\tLDA i\n\tSTA j"));
    }
    
    #[test]
    fn sixteen_bits_test1() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "short i, j, k; void main() { i = 1; j = 1; k = i + j; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCLC\n\tADC j\n\tSTA k\n\tLDA i+1\n\tADC j+1\n\tSTA k+1"));
    }
    
    #[test]
    fn if_test1() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { if (0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("JMP .ifend1\n\tLDX #1\n.ifend1"));
    }

    #[test]
    fn if_test2() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { if (!0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("main\tSUBROUTINE\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test3() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 0
        };
        let input = "unsigned char i, j; void main() { i = 0; j = 0; if (i == 0 && j == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBNE .ifend1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test4() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 0
        };
        let input = "unsigned char i, j; void main() { i = 0; j = 0; if (i == 0 || j == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n.ifstart1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test5() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 0
        };
        let input = "unsigned char i, j, k; void main() { i = 0; j = 0; k = 0; if (i == 0 || j == 0 || k == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA j\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA k\n\tCMP #0\n\tBNE .ifend1\n.ifstart1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test6() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 0
        };
        let input = "unsigned char i, j, k; void main() { i = 0; i = 0; k = 0; if (i == 0 && j == 0 && k == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBNE .ifend1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n\tLDA k\n\tCMP #0\n\tBNE .ifend1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn not_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { X = 0; Y = !X; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX #0\n\tBNE .else1\n\tLDA #1\n\tJMP .ifend1\n.else1\n\tLDA #0\n.ifend1\n\tTAY"));
    }
    
    #[test]
    fn condition_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { X = 0; Y = X == 0; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX #0\n\tBEQ .else1\n\tLDA #0\n\tJMP .ifend1\n.else1\n\tLDA #1\n.ifend1\n\tTAY"));
    }
    
    #[test]
    fn ternary_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { X = 0; Y = (X == 0)?0x10:0x20; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX #0\n\tBNE .else1\n\tLDA #16\n\tJMP .ifend1\n.else1\n\tLDA #32\n.ifend1\n\tTAY"));
    }
    
    #[test]
    fn switch_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { switch(X) { case 0: case 1: Y = 0; case 2: Y = 1; break; default: Y = 2; } }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("CPX #0\n\tBEQ .switchnextstatement2\n\tCPX #1\n\tBEQ .switchnextstatement2\n\tJMP .switchnextcase3\n.switchnextstatement2\n\tLDY #0\n\tJMP .switchnextstatement4\n.switchnextcase3\n\tCPX #2\n\tBNE .switchnextcase5\n.switchnextstatement4\n\tLDY #1\n\tJMP .switchend1\n\tJMP .switchnextstatement6\n.switchnextcase5\n.switchnextstatement6\n\tLDY #2\n.switchend1"));
    }
    
    #[test]
    fn goto_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "void main() { goto test; return; test: X = 0; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("JMP .test\n\tRTS\n.test\n\tLDX #0"));
    }
    
    #[test]
    fn assign_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "char a, b, c, d, e, f, g; void main() { g = (a+b)+(c+(d&(e=f))); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA a\n\tCLC\n\tADC b\n\tPHA\n\tLDA f\n\tSTA e\n\tLDA d\n\tAND e\n\tSTA cctmp\n\tLDA c\n\tCLC\n\tADC cctmp\n\tSTA cctmp\n\tPLA\n\tCLC\n\tADC cctmp\n\tSTA g"));
    }

    #[test]
    fn inline_test() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level: 1
        };
        let input = "inline void add() { X += Y; }; void main() { add(); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("main\tSUBROUTINE\n\tTXA\n\tCLC\n\tSTY cctmp\n\tADC cctmp\n\tTAX"));
    }

}
