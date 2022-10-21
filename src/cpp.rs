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
//! let result = cpp::process_str(text, cpp::Context::new().define("FOO", "1")).unwrap();
//!
//! assert_eq!(result, "
//!     some text
//!     more text
//!     more 1 text");
//! ```

extern crate regex;

use std::collections::BTreeMap;
use std::error;
use std::fmt;
use std::io::{self, BufRead, Write};

use log::{debug};

use regex::{Captures, Regex, Replacer};

/// The context for preprocessing a file.
///
/// Contains a list of macros and their definitions.
///
/// # Example
///
/// ```
/// let mut context = cpp::Context::new();
/// context.define("my_macro", "5");
/// assert_eq!(context.get_macro("my_macro").unwrap(), "5");
/// ```
#[derive(Debug, Clone)]
pub struct Context {
    defs: BTreeMap<String, String>,
}

/// Errors returned from preprocessing.
///
/// cpp::Error inherits from fmt::Display and so can be very easily formatted and printed.
///
/// # Example
///
/// ```
/// let error = cpp::Error::Syntax { line: 16, msg: "Invalid character." };
/// if let cpp::Error::Syntax { line, msg } = error {
///     assert_eq!(line, 16);
///     assert_eq!(msg, "Invalid character.");
/// } else {
///     panic!();
/// }
#[derive(Debug)]
pub enum Error {
    /// An error from the Rust standard I/O library.
    Io(io::Error),
    /// An error caused by malformed preprocessor syntax, with a line showing where the error
    /// occurred and a string explaining the error further.
    Syntax { line: u32, msg: &'static str },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &Error::Io(ref e) => e.fmt(f),
            &Error::Syntax { msg, line } => write!(f, "{} on line {}", msg, line),
        }
    }
}

impl error::Error for Error {
    fn cause(&self) -> Option<&dyn error::Error> {
        match self {
            &Error::Io(ref e) => Some(e),
            &Error::Syntax { .. } => None,
        }
    }
}

impl From<io::Error> for Error {
    fn from(other: io::Error) -> Self {
        Error::Io(other)
    }
}

impl Context {
    /// Creates a new, empty context with no macros defined.
    pub fn new() -> Self {
        Context {
            defs: BTreeMap::new(),
        }
    }
    /// Defines a macro within a context. As this function returns &mut Self, it can be chained
    /// like in the example.
    ///
    /// # Example
    ///
    /// ```
    /// assert_eq!(cpp::Context::new().define("foo", "bar").define("quaz", "quux").get_macro("foo").unwrap(), "bar");
    /// ```
    pub fn define<N: Into<String>, V: Into<String>>(&mut self, name: N, value: V) -> &mut Self {
        self.defs.insert(name.into(), value.into());
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
            .ok_or_else(|| Error::Syntax {
                line,
                msg: "Expected term, found nothing",
            })?
            .is_digit(10)
        {
            Ok(term == "1")
        } else {
            Err(Error::Syntax {
                line,
                msg: "Undefined identifier",
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
            return Err(Error::Syntax {
                line,
                msg: "Expected end-of-line",
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
///     bar text", cpp::Context::new().define("FOO", "1")).unwrap(), "
///     foo text
///     bar text");
/// assert_eq!(cpp::process_str("
///     #if FOO
///     foo text
///     #endif
///     bar text", cpp::Context::new().define("FOO", "0")).unwrap(), "
///     bar text");
/// ```
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
///     bar text".as_bytes(), &mut output, cpp::Context::new().define("FOO", "0"));
///
/// assert_eq!(String::from_utf8(output).unwrap(), "
///     foo text
///     more text
///     bar text");
/// ```
pub fn process<I: BufRead, O: Write>(
    mut input: I,
    mut output: O,
    context: &mut Context,
) -> Result<Vec::<u32>, Error> {
    let mut buf = String::new();
    let mut uncommented_buf = String::new();
    let mut stack = Vec::new();
    let mut state = State::Active;
    let mut lines = Vec::<u32>::new();
    let mut line = 0;
    let mut in_multiline_comments = false;
    let mut regex = context.build_regex();

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
                        if remaining.len() > 0 { insert_it = true; }
                    },
                    _ => break
                }
            } else {
                let mut s = remaining.splitn(2, "/*");
                uncommented_buf.push_str(s.next().unwrap());
                match s.next() {
                    Some(string) => {
                        in_multiline_comments = true;
                        remaining = string;
                    },
                    _ => break
                }
            }
            debug!("Line: {}, Uncommented: {:?}, Remaining: {:?}", line, uncommented_buf, remaining);
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
                let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                    line,
                    msg: "Expected something after `#ifdef`" })?;
                stack.push(state);
                if state == State::Active {
                    if context.get_macro(expr).is_none() {
                        state = State::Inactive;
                    }
                } else {
                    state = State::Skip;
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
                        "#define" => {
                            if state == State::Active {
                                let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                    line,
                                    msg: "Expected macro after `#define`" })?;
                                let mut p = expr.splitn(2, " ");
                                let mcro = p.next().unwrap();
                                let value = p.next().or_else(|| Some("")).unwrap();
                                context.define(mcro, value);
                                regex = context.build_regex();
                            } },
                        "#if" => {
                            let expr = maybe_expr.ok_or_else(|| Error::Syntax {
                                line,
                                msg: "Expected expression after `#if`" })?;
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
                                line,
                                msg: "Expected expression after `#elif`" })?;
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
                                    line,
                                    msg: "Unexpected expression after `#else`" });
                            }
                            if state == State::Inactive {
                                state = State::Active;
                            } else {
                                state = State::Skip;
                            } },
                        "#endif" => {
                            if maybe_expr.is_some() {
                                return Err(Error::Syntax {
                                    line,
                                    msg: "Unexpected expression after `#else`" });
                            }
                            state = stack.pop().ok_or_else(|| Error::Syntax {
                                line,
                                msg: "Unexpected `#endif` with no matching `#if`" })?;
                        },
                        _ => {
                            return Err(Error::Syntax {
                                line,
                                msg: "Unrecognised preprocessor directive" });
                        }
                    }
                } else if state == State::Active {
                    lines.push(line);
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
                &mut Context::new()
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "1")
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
                &mut Context::new()
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
                &mut Context::new()
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
                Context::new().define("FOO", "1")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "1")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "1")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
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
                Context::new().define("FOO", "0")
            )
            .unwrap(),
            "
            some
            FOO_BAR
            multiline
        "
        );
    }
}

