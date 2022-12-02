//! C-like generic preprocessor for Rust. It supports macros, #define, #if, #elif, #else and
//! #endif.
//!
//! Process text with the `process` and `process_str` functions.
//!
//! # Examples
//!
//! ```
//! let text = "
//!     some text
//!     #if FOO
//!     more text
//!     #endif
//!     more FOO text";
//!
//! let result = cpp::process_str(text, cpp::Context::new("string").define("FOO", "1")).unwrap();
//!
//! assert_eq!(result, "
//!     some text
//!     more text
//!     more 1 text");
//! ```

use crate::error::Error;

extern crate regex;

use std::collections::BTreeMap;
use std::io::{BufReader, BufRead, Write};
use std::fs::File;
use std::path::Path;

use log::debug;

use regex::{Captures, Regex, Replacer};

/// The context for preprocessing a file.
///
/// Contains a list of macros and their definitions.
///
/// # Example
///
/// ```
/// let mut context = cpp::Context::new("string");
/// context.define("my_macro", "5");
/// assert_eq!(context.get_macro("my_macro").unwrap(), "5");
/// ```
#[derive(Debug, Clone)]
pub struct Context {
    current_filename: String,
    includes_stack: Vec<(String, u32)>,
    pub include_directories: Vec<String>,
    defs: BTreeMap<String, String>,
}

impl Context {
    /// Creates a new, empty context with no macros defined.
    pub fn new(current_filename: &str) -> Self {
        Context {
            current_filename: String::from(current_filename),
            includes_stack: Vec::<(String, u32)>::new(),
            include_directories: Vec::<String>::new(),
            defs: BTreeMap::new(),
        }
    }
    /// Defines a macro within a context. As this function returns &mut Self, it can be chained
    /// like in the example.
    ///
    /// # Example
    ///
    /// ```
    /// assert_eq!(cpp::Context::new("string").define("foo", "bar").define("quaz", "quux").get_macro("foo").unwrap(), "bar");
    /// ```
    pub fn define<N: Into<String>, V: Into<String>>(&mut self, name: N, value: V) -> &mut Self {
        self.defs.insert(name.into(), value.into());
        self
    }
    /// 
    pub fn undefine<N: Into<String>>(&mut self, name: N) -> &mut Self {
        self.defs.remove(&name.into());
        self
    }
    /// Gets a macro that may or may not be defined from a context.
    pub fn get_macro<N: Into<String>>(&self, name: N) -> Option<&String> {
        self.defs.get(&name.into())
    }
    fn build_regex(&self) -> Regex {
        if self.defs.is_empty() {
            Regex::new("$_").expect("Regex should be valid")
        } else {
            let pat: String = self
                .defs
                .keys()
                .flat_map(|k| vec!["|", &k])
                .skip(1)
                .collect();
            Regex::new(&format!("\\b(?:{})\\b", pat)).expect("Regex should be valid")
        }
    }
    fn replacer<'a>(&'a self) -> impl Replacer + 'a {
        move |captures: &Captures| {
            self.defs
                .get(captures.get(0).expect("At least one capture").as_str())
                .expect("Found def for match")
                .clone()
        }
    }
    fn skip_whitespace(&self, expr: &mut &str) {
        *expr = expr.trim_start();
    }
    fn eval_term(&self, expr: &mut &str, line: u32) -> Result<bool, Error> {
        self.skip_whitespace(expr);

        let index = expr
            .find(|c: char| !c.is_ascii_alphanumeric() && c != '_')
            .unwrap_or(expr.len());
        let term = &expr[0..index];
        *expr = &expr[index..];

        if term
            .chars()
            .next()
            .ok_or_else(|| { 
                let filename = self.current_filename.clone();
                let included_in = match self.includes_stack.last() {
                    Some(s) => Some(s.clone()),
                    None => None,
                };
                Error::Syntax {
                    filename, included_in, line,
                    msg: "Expected term, found nothing".to_string() }
            })?
        .is_digit(10)
        {
            Ok(term == "1")
        } else {
            let filename = self.current_filename.clone();
            let included_in = match self.includes_stack.last() {
                Some(s) => Some(s.clone()),
                None => None,
            };
            Err(Error::Syntax {
                filename, included_in, line,
                msg: "Undefined identifier".to_string(),
            })
        }
    }
    fn eval_unary(&self, expr: &mut &str, line: u32) -> Result<bool, Error> {
        let mut negate = false;
        self.skip_whitespace(expr);
        while expr.starts_with("!") {
            *expr = &expr[1..];
            negate = !negate;
            self.skip_whitespace(expr);
        }

        Ok(negate ^ self.eval_term(expr, line)?)
    }
    fn eval_eq(&self, expr: &mut &str, line: u32) -> Result<bool, Error> {
        let mut result = self.eval_unary(expr, line)?;
        self.skip_whitespace(expr);
        while expr.starts_with("==") {
            *expr = &expr[2..];
            result ^= !self.eval_unary(expr, line)?;
            self.skip_whitespace(expr);
        }
        Ok(result)
    }
    fn evaluate(&self, mut expr: &str, line: u32) -> Result<bool, Error> {
        let result = self.eval_eq(&mut expr, line)?;
        self.skip_whitespace(&mut expr);
        if !expr.is_empty() {
            let filename = self.current_filename.clone();
            let included_in = match self.includes_stack.last() {
                Some(s) => Some(s.clone()),
                None => None,
            };
            return Err(Error::Syntax {
                filename, included_in, line,
                msg: "Expected end-of-line".to_string(),
            });
        }
        Ok(result)
    }
}

#[derive(Eq, PartialEq, Debug, Copy, Clone)]
enum State {
    // A condition already matched, skip remaining clauses
    Skip,
    // Condition has not yet matched, evaluate remaining clauses
    Inactive,
    // Condition currently matches, pass through input
    Active,
}

/// Preprocesses a string.
///
/// This function takes a context and a string, and preprocesses it.
///
/// # Errors
///
/// This function returns a result and can fail with Err(cpp::Error).
///
/// # Examples
///
/// ```
/// assert_eq!(cpp::process_str("
///     #if FOO
///     foo text
///     #endif
///     bar text", cpp::Context::new("string").define("FOO", "1")).unwrap(), "
///     foo text
///     bar text");
/// assert_eq!(cpp::process_str("
///     #if FOO
///     foo text
///     #endif
///     bar text", cpp::Context::new("string").define("FOO", "0")).unwrap(), "
///     bar text");
/// ```
#[allow(dead_code)]
pub fn process_str(input: &str, context: &mut Context) -> Result<String, Error> {
    let mut output = Vec::new();
    process(input.as_bytes(), &mut output, context)?;
    Ok(String::from_utf8(output).expect("Input was utf8, so output should be too..."))
}

/// Preprocesses a generic buffer.
///
/// This function takes any generic BufRead input and Write output and preprocesses it.
///
/// # Example
///
/// ```
/// let mut output = Vec::new();
/// cpp::process("
///     foo text
///     #if !FOO
///     more text
///     #endif
///     bar text".as_bytes(), &mut output, cpp::Context::new("string").define("FOO", "0"));
///
/// assert_eq!(String::from_utf8(output).unwrap(), "
///     foo text
///     more text
///     bar text");
/// ```
pub fn process<I: BufRead, O: Write>(
    mut input: I,
    output: &mut O,
    context: &mut Context,
) -> Result<Vec::<(std::rc::Rc::<String>,u32,Option<(std::rc::Rc::<String>,u32)>)>, Error> {
    let mut buf = String::new();
    let mut uncommented_buf = String::new();
    let mut stack = Vec::new();
    let mut state = State::Active;
    let mut lines = Vec::<(std::rc::Rc::<String>,u32,Option<(std::rc::Rc::<String>,u32)>)>::new();
    let mut line = 0;
    let filename = context.current_filename.clone();
    let filename_rc = std::rc::Rc::<String>::new(filename.clone());
    let mut in_multiline_comments = false;
    let mut regex = context.build_regex();

    let (included_in, included_in_rc) = match context.includes_stack.last() {
        Some(s) => (Some(s.clone()), Some((std::rc::Rc::<String>::new(s.0.clone()), s.1.clone()))),
        None => (None, None),
    };

    while input.read_line(&mut buf)? > 0 {
        line += 1;
        let has_lf = buf.ends_with("\n");
        let mut remaining: &str = &buf;
        let mut insert_it = !in_multiline_comments;
        uncommented_buf.clear();
        while remaining.len() > 0 {
            if in_multiline_comments {
                let mut s = remaining.splitn(2, "*/");
                s.next().unwrap();
                match s.next() {
                    Some(string) => {
                        in_multiline_comments = false;
                        remaining = string;
                        if remaining.len() > 0 { 
                            if remaining.eq("\n") {
                                remaining = "";
                            } else {
                                insert_it = true; 
                            }
                        }
                    },
                    _ => break
                }
            } else {
                let mut s = remaining.splitn(2, "//").next().unwrap().splitn(2, "/*");
                uncommented_buf.push_str(s.next().unwrap());
                if uncommented_buf.is_empty() { insert_it = false; }
                match s.next() {
                    Some(string) => {
                        in_multiline_comments = true;
                        remaining = string;
                    },
                    _ => break
                }
            }
            debug!("Line: {}, Uncommented: {:?}, Remaining: {:?}, insert it: {:?}", line, uncommented_buf, remaining, insert_it);
        }
        if insert_it {
            let substr = uncommented_buf.trim();
            // Before substitution, test the #ifdef
            if substr.starts_with("#ifdef") {
                let mut parts = substr.splitn(2, "//").next().unwrap().splitn(2, " ");
                parts.next().unwrap();
                let maybe_expr = parts.next().map(|s| s.trim()).and_then(|s| {
                    if s.is_empty() {
                        None
                    } else {
                        Some(s)
                    }
                });
                let expr = match maybe_expr {
                    Some(x) => x,
                    _ => {
                        return Err(Error::Syntax {
                            filename: filename.clone(), included_in: included_in.clone(), line,
                            msg: "Expected something after `#ifdef`".to_string() })

                    }
                };
                stack.push(state);
                if state == State::Active {
                    if context.get_macro(expr).is_none() {
                        state = State::Inactive;
                    }
                } else {
                    state = State::Skip;
                }
            } else if substr.starts_with("#ifndef") {
                let mut parts = substr.splitn(2, "//").next().unwrap().splitn(2, " ");
                parts.next().unwrap();
                let maybe_expr = parts.next().map(|s| s.trim()).and_then(|s| {
                    if s.is_empty() {
                        None
                    } else {
                        Some(s)
                    }
                });
                let expr = match maybe_expr {
                    Some(x) => x,
                    _ => {
                        return Err(Error::Syntax {
                            filename: filename.clone(), included_in: included_in.clone(), line,
                            msg: "Expected something after `#ifndef`".to_string() })
                        }
                };
                stack.push(state);
                if state == State::Active {
                    if context.get_macro(expr).is_some() {
                        state = State::Inactive;
                    }
                } else {
                    state = State::Skip;
                }
            } else if substr.starts_with("#undef") {
                if state == State::Active {
                    let mut parts = substr.splitn(2, "//").next().unwrap().splitn(2, " ");
                    parts.next().unwrap();
                    let maybe_expr = parts.next().map(|s| s.trim()).and_then(|s| {
                        if s.is_empty() {
                            None
                        } else {
                            Some(s)
                        }
                    });
                    let expr = match maybe_expr {
                        Some(x) => x,
                        _ => {
                            return Err(Error::Syntax {
                                filename: filename.clone(), included_in: included_in.clone(), line,
                                msg: "Expected something after `#undef`".to_string() })
                        }
                    };

                    if context.get_macro(expr).is_none() {
                        return Err(Error::Syntax {
                            filename: filename.clone(), included_in: included_in.clone(), line,
                            msg: format!("Macro {} is not defined", expr)});
                    }
                    context.undefine(expr);
                    regex = context.build_regex();
                } 
            } else { 
                let substr;
                let new_line;
                {
                    let mut replacer = context.replacer();
                    new_line = regex.replace_all(&uncommented_buf, replacer.by_ref());
                    substr = new_line.trim();
                }
                if substr.starts_with("#") {
                    let mut parts = substr.splitn(2, "//").next().unwrap().splitn(2, " ");
                    let name = parts.next().unwrap();
                    let maybe_expr = parts.next().map(|s| s.trim()).and_then(|s| {
                        if s.is_empty() {
                            None
                        } else {
                            Some(s)
                        }
                    });

                    match name {
                        "#include" => {
                            if state == State::Active {
                                // Get filename
                                let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                    filename: filename.clone(), included_in: included_in.clone(), line,
                                    msg: "Expected filename after `#include`".to_string() })?;
                                let mut chars = expr.chars();
                                let separator = chars.next();
                                let end_separator = match separator {
                                    Some('<') => '>',
                                    Some('"') => '"',
                                    _ => return Err(Error::Syntax { filename: filename.clone(), included_in: included_in.clone(), line, msg: "Expected < or \" in #include filename spec".to_string() })
                                };
                                let mut fname = String::new();
                                loop {
                                    let nc = chars.next();
                                    match nc {
                                        Some(x) => if x == end_separator { break; } else { fname.push(x); }, 
                                        None => return Err(Error::Syntax { filename: filename.clone(), included_in: included_in.clone(), line, msg: "Missing end separator in #include fname".to_string() })
                                    }    
                                }

                                // Open include file
                                let mut px;
                                let mut path = Path::new(&fname);
                                if !path.exists() {
                                    let mut found = false;
                                    for p in &context.include_directories {
                                        px = p.clone() + "/" + &fname;
                                        path = Path::new(&px);
                                        if path.exists() {
                                            found = true;
                                            break;
                                        }
                                    }    
                                    if !found {
                                        return Err(Error::Syntax { 
                                            filename: filename.clone(), included_in: included_in.clone(), line, msg: format!("Included file {fname} not found")});
                                    }
                                }

                                // Process file
                                let f = File::open(path)?;
                                let f = BufReader::new(f);
                                context.current_filename = fname.clone();
                                context.includes_stack.push((filename.clone(), line));
                                let mut mapped_lines = process(f, output, context)?;
                                context.includes_stack.pop();
                                context.current_filename = filename.clone();
                                lines.append(&mut mapped_lines);
                            } },
                        "#define" => {
                            if state == State::Active {
                                let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                    filename: filename.clone(), included_in: included_in.clone(), line,
                                    msg: "Expected macro after `#define`".to_string() })?;
                                let mut p = expr.splitn(2, " ");
                                let mcro = p.next().unwrap();
                                let value = p.next().or_else(|| Some("")).unwrap();
                                if context.get_macro(mcro).is_some() {
                                    return Err(Error::Syntax {
                                        filename: filename.clone(), included_in: included_in.clone(), line,
                                        msg: format!("Macro {} already defined", mcro)});
                                }
                                context.define(mcro, value);
                                regex = context.build_regex();
                            } },
                        "#if" => {
                            let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                filename: filename.clone(), included_in: included_in.clone(), line,
                                msg: "Expected expression after `#if`".to_string() })?;
                            stack.push(state);
                            if state == State::Active {
                                if !context.evaluate(expr, line)? {
                                    state = State::Inactive;
                                }
                            } else {
                                state = State::Skip;
                            } },
                        "#elif" => {
                            let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                filename: filename.clone(), included_in: included_in.clone(), line, 
                                msg: "Expected expression after `#elif`".to_string() })?;
                            if state == State::Inactive {
                                if context.evaluate(expr, line)? {
                                    state = State::Active;
                                }
                            } else {
                                state = State::Skip;
                            } },
                        "#else" => {
                            if maybe_expr.is_some() {
                                return Err(Error::Syntax {
                                    filename: filename.clone(), included_in: included_in.clone(), line,
                                    msg: "Unexpected expression after `#else`".to_string() });
                            }
                            if state == State::Inactive {
                                state = State::Active;
                            } else {
                                state = State::Skip;
                            } },
                        "#endif" => {
                            if maybe_expr.is_some() {
                                return Err(Error::Syntax {
                                    filename: filename.clone(), included_in: included_in.clone(), line,
                                    msg: "Unexpected expression after `#else`".to_string() });
                            }
                            state = stack.pop().ok_or_else(|| Error::Syntax {
                                filename: filename.clone(), included_in: included_in.clone(), line,
                                msg: "Unexpected `#endif` with no matching `#if`".to_string() })?;
                        },
                        "#error" => {
                            let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                filename: filename.clone(), included_in: included_in.clone(), line,
                                msg: "Expected error text after `#error`".to_string() })?;
                            return Err(Error::Compiler {
                                filename: filename.clone(), included_in: included_in.clone(), line,
                                msg: expr.to_string() });
                        },
                        _ => {
                            return Err(Error::Syntax {
                                filename: filename.clone(), included_in: included_in.clone(), line,
                                msg: "Unrecognised preprocessor directive".to_string() });
                        }
                    }
                } else if state == State::Active {
                    lines.push((filename_rc.clone(), line, included_in_rc.clone()));
                    output.write_all(new_line.as_bytes())?;
                    if !new_line.ends_with("\n") && has_lf { output.write(b"\n")?; }
                }
            } 
        }
        buf.clear();
    }
    Ok(lines)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn pass_through() {
        assert_eq!(
            &process_str(
                "
            some
            multiline
            text
            with # symbols
        ",
                &mut Context::new("string")
            )
            .unwrap(),
            "
            some
            multiline
            text
            with # symbols
        "
        );
    }

    #[test]
    fn variable() {
        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "1")
            )
            .unwrap(),
            "
            some
            multiline
            text
            with # symbols
        "
        );
    }

    #[test]
    fn constant() {
        assert_eq!(
            &process_str(
                "
            some
            #if 0
            multiline
            text
            #endif
            with # symbols
        ",
                &mut Context::new("string")
            )
            .unwrap(),
            "
            some
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if 1
            multiline
            text
            #endif
            with # symbols
        ",
                &mut Context::new("string")
            )
            .unwrap(),
            "
            some
            multiline
            text
            with # symbols
        "
        );
    }

    #[test]
    fn negation() {
        assert_eq!(
            &process_str(
                "
            some
            #if !FOO
            multiline
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "1")
            )
            .unwrap(),
            "
            some
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if !FOO
            multiline
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            multiline
            text
            with # symbols
        "
        );
    }

    #[test]
    fn else_() {
        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #else
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            text
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #else
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "1")
            )
            .unwrap(),
            "
            some
            multiline
            with # symbols
        "
        );
    }

    #[test]
    fn elif() {
        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #elif 1
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            text
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #elif 1
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "1")
            )
            .unwrap(),
            "
            some
            multiline
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #elif 0
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #elif 0
            text
            #else
            with # symbols
            #endif
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO
            multiline
            #elif 1
            text
            #else
            with # symbols
            #endif
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            text
        "
        );
    }

    #[test]
    fn equality() {
        assert_eq!(
            &process_str(
                "
            some
            #if FOO == 1
            multiline
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            with # symbols
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            #if FOO == 0
            multiline
            text
            #endif
            with # symbols
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            multiline
            text
            with # symbols
        "
        );
    }

    #[test]
    fn expansion() {
        assert_eq!(
            &process_str(
                "
            some
            FOO-BAR
            multiline
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            0-BAR
            multiline
        "
        );

        assert_eq!(
            &process_str(
                "
            some
            FOO_BAR
            multiline
        ",
                Context::new("string").define("FOO", "0")
            )
            .unwrap(),
            "
            some
            FOO_BAR
            multiline
        "
        );
    }
    
    #[test]
    fn multiline_comments() {
        assert_eq!(process_str("
     #if FOO
     foo text /* Bobby */
     #endif
     bar text", Context::new("string").define("FOO", "1")).unwrap(), "
     foo text 
     bar text");

    }

    #[test]
    fn define() {
    assert_eq!(process_str("#define ZOBI
     #define FOO FOO_BAR // JR
     #ifdef FOO
     foo text
     #endif
     FOO /*bar text
     Dallas
     */ Ewing", &mut Context::new("string")).unwrap(), "     foo text
     FOO_BAR 
 Ewing");
    }

    #[test]
    fn lines_mapping() {
        let mut output = Vec::new();
        let result = process("#define ZOBI
            #define FOO FOO_BAR 
            #ifdef FOO
                foo text
            #endif
            FOO bar text".as_bytes(), &mut output, &mut Context::new("string"));
        assert_eq!(result.unwrap().iter().map(|x| x.1).collect::<Vec::<u32>>(), &[4, 6]);
    }
    
    #[test]
    fn lines_mapping2() {
        let mut output = Vec::new();
        let result = process("/* Hello */
            world".as_bytes(), &mut output, &mut Context::new("string"));
        assert_eq!(result.unwrap().iter().map(|x| x.1).collect::<Vec::<u32>>(), &[2]);
    }
    
    #[test]
    fn error() {
        let mut context = Context::new("string");
        context.current_filename = "string".to_string();
        let result = process_str("#error This is an error
            foo bar", &mut context);
        assert_eq!(result.err().unwrap().to_string(), 
            "Compiler error: This is an error on line 1 of string".to_string()
            );
    }
}

