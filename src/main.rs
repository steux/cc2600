mod cpp;
mod error;
mod compile;
mod generate;

use std::fs::File;
use std::io::BufReader;
use compile::compile;

extern crate pest;
#[macro_use]
extern crate pest_derive;

use clap::Parser as ClapParser;

/// Subset of C compiler for the Atari 2600
#[derive(ClapParser, Debug)]
#[command(author, version, about, long_about = None)]
pub struct Args {
    /// Input file name
    input: String,

    /// Preprocessor definitions
    #[arg(short='D')]
    defines: Vec<String>,

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
            insert_code: false
        };
        let input = "unsigned char i; void main() { for (i = 0; i < 10; i++); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tLDA i\n\tCMP #10\n\tBCS .forend1\n.for1\n.forupdate1\n\tINC i\n\tLDA i\n\tCMP #10\n\tBCC .for1\n.forend1\n"));
    }
    
    #[test]
    fn plusplus_statement_test1() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
        };
        let input = "unsigned char i, j; void main() { i = 0; j = i++; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tLDA i\n\tSTA j\n\tINC i\n"));
    }
    
    #[test]
    fn plusplus_statement_test2() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
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
            insert_code: false
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
            insert_code: false
        };
        let input = "void main() { if (0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("JMP .ifend1\n\tLDX #1\n.ifend1\n\tRTS"));
    }

    #[test]
    fn if_test2() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
        };
        let input = "void main() { if (!0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("main\tSUBROUTINE\n\tLDX #1\n.ifend1\n\tRTS"));
    }
    
    #[test]
    fn if_test3() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
        };
        let input = "unsigned char i, j; void main() { i = 0; i = 0; if (i == 0 && j == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBNE .ifend1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n\tLDX #1\n.ifend1\n\tRTS"));
    }
    
    #[test]
    fn if_test4() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
        };
        let input = "unsigned char i, j; void main() { i = 0; i = 0; if (i == 0 || j == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n.ifstart1\n\tLDX #1\n.ifend1\n\tRTS"));
    }
    
    #[test]
    fn if_test5() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
        };
        let input = "unsigned char i, j, k; void main() { i = 0; i = 0; k = 0; if (i == 0 || j == 0 || k == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA j\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA k\n\tCMP #0\n\tBNE .ifend1\n.ifstart1\n\tLDX #1\n.ifend1\n\tRTS"));
    }
    
    #[test]
    fn if_test6() {
        let args = Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false
        };
        let input = "unsigned char i, j, k; void main() { i = 0; i = 0; k = 0; if (i == 0 && j == 0 && k == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBNE .ifend1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n\tLDA k\n\tCMP #0\n\tBNE .ifend1\n\tLDX #1\n.ifend1\n\tRTS"));
    }
}
