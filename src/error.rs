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
    Unimplemented { feature: &'static str },
    Configuration { error: String }
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
            Error::Unimplemented { feature } => {
                write!(f, "Compiler error: Unimplemented feature - {}", feature)
            },
            Error::Configuration { error } => {
                write!(f, "Compiler error: Bad configuration - {}", error)
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
            &Error::Unimplemented { .. } => None,
            &Error::Configuration { .. } => None,
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
