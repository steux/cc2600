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
    insert_code: bool,

    /// Set char signedness to signed
    #[arg(long("fsigned_char"), default_value = "false")]
    signed_chars: bool,
    
    /// Set char signedness to unsigned (default)
    #[arg(long("funsigned_char"), default_value = "true")]
    unsigned_chars: bool
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

    fn sargs(optimization_level: u8) -> Args {
        Args {
            input: "string".to_string(),
            output: "string".to_string(),
            include_directories: Vec::new(),
            defines: Vec::new(),
            insert_code: false,
            verbose: false,
            optimization_level,
            signed_chars: false,
            unsigned_chars: true,
        }
    }

    #[test]
    fn for_statement_test1() {
        let args = sargs(1); 
        let input = "unsigned char i; void main() { for (i = 0; i < 10; i++); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tCMP #10\n\tBCS .forend1\n.for1\n.forupdate1\n\tINC i\n\tLDA i\n\tCMP #10\n\tBCC .for1\n.forend1\n"));
    }
    
    #[test]
    fn for_statement_test2() {
        let args = sargs(1); 
        let input = "unsigned char i; void main() { for (i = 0; i != 10; i++); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n.for1\n.forupdate1\n\tINC i\n\tLDA i\n\tCMP #10\n\tBNE .for1\n.forend1\n"));
    }
    
    #[test]
    fn plusplus_statement_test1() {
        let args = sargs(1); 
        let input = "unsigned char i, j; void main() { i = 0; j = i++; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tSTA j\n\tINC i\n"));
    }
    
    #[test]
    fn plusplus_statement_test2() {
        let args = sargs(1); 
        let input = "unsigned char i, j; void main() { i = 0; j = ++i; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tINC i\n\tLDA i\n\tSTA j"));
    }
    
    #[test]
    fn sixteen_bits_test1() {
        let args = sargs(1);
        let input = "short i, j, k; void main() { i = 1; j = 1; k = i + j; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCLC\n\tADC j\n\tSTA k\n\tLDA i+1\n\tADC j+1\n\tSTA k+1"));
    }
    
    #[test]
    fn sixteen_bits_test2() {
        let args = sargs(1);
        let input = "unsigned short i; unsigned char j; void main() { i = j; i = j << 8; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA j\n\tSTA i\n\tLDA #0\n\tSTA i+1\n\tSTA i\n\tLDA j\n\tSTA i+1"));
    }
    
    #[test]
    fn if_test1() {
        let args = sargs(1);
        let input = "void main() { if (0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("JMP .ifend1\n\tLDX #1\n.ifend1"));
    }

    #[test]
    fn if_test2() {
        let args = sargs(1);
        let input = "void main() { if (!0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("main\tSUBROUTINE\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test3() {
        let args = sargs(0);
        let input = "unsigned char i, j; void main() { i = 0; j = 0; if (i == 0 && j == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBNE .ifend1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test4() {
        let args = sargs(0);
        let input = "unsigned char i, j; void main() { i = 0; j = 0; if (i == 0 || j == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n.ifstart1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test5() {
        let args = sargs(0);
        let input = "unsigned char i, j, k; void main() { i = 0; j = 0; k = 0; if (i == 0 || j == 0 || k == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA j\n\tCMP #0\n\tBEQ .ifstart1\n\tLDA k\n\tCMP #0\n\tBNE .ifend1\n.ifstart1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn if_test6() {
        let args = sargs(0);
        let input = "unsigned char i, j, k; void main() { i = 0; i = 0; k = 0; if (i == 0 && j == 0 && k == 0) X = 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #0\n\tBNE .ifend1\n\tLDA j\n\tCMP #0\n\tBNE .ifend1\n\tLDA k\n\tCMP #0\n\tBNE .ifend1\n\tLDX #1\n.ifend1"));
    }
    
    #[test]
    fn not_test() {
        let args = sargs(1);
        let input = "void main() { X = 0; Y = !X; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX #0\n\tBNE .else1\n\tLDA #1\n\tJMP .ifend1\n.else1\n\tLDA #0\n.ifend1\n\tTAY"));
    }
    
    #[test]
    fn condition_test() {
        let args = sargs(1);
        let input = "void main() { X = 0; Y = X == 0; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX #0\n\tBEQ .else1\n\tLDA #0\n\tJMP .ifend1\n.else1\n\tLDA #1\n.ifend1\n\tTAY"));
    }
    
    #[test]
    fn ternary_test() {
        let args = sargs(1);
        let input = "void main() { X = 0; Y = (X == 0)?0x10:0x20; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX #0\n\tBNE .else1\n\tLDA #16\n\tJMP .ifend1\n.else1\n\tLDA #32\n.ifend1\n\tTAY"));
    }
    
    #[test]
    fn switch_test() {
        let args = sargs(1);
        let input = "void main() { switch(X) { case 0: case 1: Y = 0; case 2: Y = 1; break; default: Y = 2; } }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("CPX #0\n\tBEQ .switchnextstatement2\n\tCPX #1\n\tBEQ .switchnextstatement2\n\tJMP .switchnextcase3\n.switchnextstatement2\n\tLDY #0\n\tJMP .switchnextstatement4\n.switchnextcase3\n\tCPX #2\n\tBNE .switchnextcase5\n.switchnextstatement4\n\tLDY #1\n\tJMP .switchend1\n\tJMP .switchnextstatement6\n.switchnextcase5\n.switchnextstatement6\n\tLDY #2\n.switchend1"));
    }
    
    #[test]
    fn goto_test() {
        let args = sargs(1);
        let input = "void main() { goto test; return; test: X = 0; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("JMP .test\n\tRTS\n.test\n\tLDX #0"));
    }
    
    #[test]
    fn assign_test() {
        let args = sargs(1);
        let input = "char a, b, c, d, e, f, g; void main() { g = (a+b)+(c+(d&(e=f))); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA a\n\tCLC\n\tADC b\n\tPHA\n\tLDA f\n\tSTA e\n\tLDA d\n\tAND e\n\tSTA cctmp\n\tLDA c\n\tCLC\n\tADC cctmp\n\tSTA cctmp\n\tPLA\n\tCLC\n\tADC cctmp\n\tSTA g"));
    }

    #[test]
    fn inline_test() {
        let args = sargs(1);
        let input = "inline void add() { X += Y; }; void main() { add(); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("main\tSUBROUTINE\n\tTXA\n\tCLC\n\tSTY cctmp\n\tADC cctmp\n\tTAX"));
    }

    #[test]
    fn inline_test2() {
        let args = sargs(1);
        let input = "inline void w() { while(Y) Y--; }; void main() { w(); w(); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains(".while1inline1\n\tCPY #0\n\tBEQ .whileend1inline1\n\tDEY\n\tJMP .while1inline1\n.whileend1inline1\n.while1inline2\n\tCPY #0\n\tBEQ .whileend1inline2\n\tDEY\n\tJMP .while1inline2\n.whileend1inline2"));
    }

    #[test]
    fn array_of_pointers_test() {
        let args = sargs(1);
        let input = "
const char s1[] = {0};
const char s2[] = {0};
const char *ss[] = {s1, s2};

char *ptr;
char v;

void main()
{
    ptr = ss[X];
    v = ptr[Y];
}
            ";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA ss,X\n\tSTA ptr\n\tLDA ss+2,X\n\tSTA ptr+1\n\tLDA (ptr),Y\n\tSTA v"));
    }
    
    #[test]
    fn array_of_pointers_test2() {
        let args = sargs(1);
        let input = "
const char s1[] = {0};
const char s2[] = {0};
const char *ss[] = {s1, s2};

char *ptr;
char v;

void main()
{
    ptr = ss[0];
    v = ptr[Y];
}
            ";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA ss\n\tSTA ptr\n\tLDA ss+2\n\tSTA ptr+1\n\tLDA (ptr),Y\n\tSTA v"));
    }
    
    #[test]
    fn array_of_pointers_test3() {
        let args = sargs(1);
        let input = "
char *s1, *s2;
char *ss[2];

char *ptr;
char v;

void main()
{
    ss[0] = s1;
    ss[1] = s2;
    ptr = ss[1];
    v = ptr[Y];
}
            ";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA s1\n\tSTA ss\n\tLDA s1+1\n\tSTA ss+2\n\tLDA s2\n\tSTA ss+1\n\tLDA s2+1\n\tSTA ss+3\n\tLDA ss+1\n\tSTA ptr\n\tLDA ss+3\n\tSTA ptr+1\n\tLDA (ptr),Y\n\tSTA v"));
    }
    
    #[test]
    fn longbranch_test() {
        let args = sargs(1);
        let mut input: String = "void main() { do {".to_string();
        for _ in 0..130 {
            input.push_str("csleep(2);");
        }
        input.push_str("} while (Y);}");
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("NOP\n.dowhilecondition1\n\tCPY #0\n\tBEQ .fix1\n\tJMP .dowhile1\n.fix1\n.dowhileend1"));
    }

    #[test]
    fn load_test() {
        let args = sargs(1);
        let input = "char i; void main() { i = 0; load(0); }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #0\n\tSTA i\n\tLDA #0"));
    }
    
    #[test]
    fn calc_test() {
        let args = sargs(1);
        let input = "const char tab[1 + 1] = {3 * 5, 4 << 2};";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("hex 0f10"));
    }
    
    #[test]
    fn array_indexing_test() {
        let args = sargs(1);
        let input = "char c[2]; char i; void main() { c[X = i] = 0; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDX i\n\tLDA #0\n\tSTA c,X"));
    }
    
    #[test]
    fn sixteen_bits_minusminus_test() {
        let args = sargs(1);
        let input = "short i; void main() { i--; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tSEC\n\tSBC #1\n\tSTA i\n\tLDA i+1\n\tSBC #0\n\tSTA i+1"));
    }
    
    #[test]
    fn sixteen_bits_increment_test() {
        let args = sargs(1);
        let input = "short i; void main() { i += -1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCLC\n\tADC #255\n\tSTA i\n\tLDA i+1\n\tADC #255\n\tSTA i+1"));
    }
    
    #[test]
    fn sign_extend_test() {
        let args = sargs(1);
        let input = "short i; signed char x; void main() { i += x; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCLC\n\tADC x\n\tSTA i\n\tLDA x\n\tORA #127\n\tBMI .ifneg1\n\tLDA #0\n.ifneg1\n\tADC i+1\n\tSTA i+1"));
    }
    
    #[test]
    fn splices_test() {
        let args = sargs(1);
        let input = "#define one \\
1
char i; void main() { i = one; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA #1\n\tSTA i"));
    }
    
    #[test]
    fn quoted_string_test1() {
        let args = sargs(1);
        let input = "char *s; void main() { s = \"zobi\\\"\\\\\"; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("cctmp0\n\thex 7a6f6269225c0"));
    }
    
    #[test]
    fn quoted_string_test2() {
        let args = sargs(1);
        let input = "char *s = \"\tzobi\\\"\\\\\";";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("s\n\thex 097a6f6269225c00"));
    }
    
    #[test]
    fn quoted_string_test3() {
        let args = sargs(1);
        let input = "char *s[2] = {\"zobi\", \"zoba\"};";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("cctmp0\n\thex 7a6f626900\ncctmp1\n\thex 7a6f626100\ns\n\t.byte <cctmp0, <cctmp1, >cctmp0, >cctmp1"));
    }
    
    #[test]
    fn right_shift_test1() {
        let args = sargs(1);
        let input = "signed char i; void main() { i >>= 1; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tCMP #128\n\tROR\n\tSTA i"));
    }

    #[test]
    fn right_shift_test2() {
        let args = sargs(1);
        let input = "signed char i; void main() { i >>= 2; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA i\n\tLSR\n\tLSR\n\tCLC\n\tADC #224\n\tEOR #224\n\tSTA i"));
    }
    
    #[test]
    fn right_shift_test3() {
        let args = sargs(1);
        let input = "char i; char *ptr; void main() { i = ptr >> 8; }";
        let mut output = Vec::new();
        compile(input.as_bytes(), &mut output, &args).unwrap();
        let result = str::from_utf8(&output).unwrap();
        print!("{:?}", result);
        assert!(result.contains("LDA ptr+1\n\tSTA i"));
    }
}
