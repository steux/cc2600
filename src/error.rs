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

impl From<std::str::Utf8Error> for Error {
    fn from(_other: std::str::Utf8Error) -> Self {
        Error::Syntax {
                line: 0,
                msg: "Utf8 conversion error",
            }
    }
}
