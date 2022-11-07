use std::error;
use std::fmt;
use std::io;

/// Errors returned from preprocessing and compilation.
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
pub enum Error  {
    /// An error from the Rust standard I/O library.
    Io(io::Error),
    /// An error caused by malformed preprocessor syntax, with a line showing where the error
    /// occurred and a string explaining the error further.
    Syntax { filename: String, included_in: Option<(String, u32)>, line: u32, msg: String },
    Compiler { filename: String, included_in: Option<(String, u32)>, line: u32, msg: String },
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            &Error::Io(ref e) => e.fmt(f),
            Error::Syntax { filename, included_in, msg, line } => {
                match included_in {
                    Some(include) => write!(f, "Syntax error: {} on line {} of {} (included in {} on line {})", msg, line, filename, include.0, include.1),
                    None => write!(f, "Syntax error: {} on line {} of {}", msg, line, filename)
                }
            },
            Error::Compiler { filename, included_in, line, msg } => {
                match included_in {
                    Some(include) => write!(f, "Compiler error: {} on line {} of {} (included in {} on line {})", msg, line, filename, include.0, include.1),
                    None => write!(f, "Compiler error: {} on line {} of {}", msg, line, filename)
                }
            },
        }
    }
}

impl error::Error for Error {
    fn cause(&self) -> Option<&dyn error::Error> {
        match self {
            &Error::Io(ref e) => Some(e),
            &Error::Syntax { .. } => None,
            &Error::Compiler { .. } => None,
        }
    }
}

impl From<io::Error> for Error {
    fn from(other: io::Error) -> Self {
        Error::Io(other)
    }
}

impl From<std::str::Utf8Error> for Error {
    fn from(_other: std::str::Utf8Error) -> Self {
        Error::Syntax {
                filename: "internal buffer".to_string(),
                included_in: None,
                line: 0,
                msg: "Utf8 conversion error".to_string(),
            }
    }
}
