# Contributing to Olive

Thank you for your interest in contributing to Olive!

## Reporting issues

Bug reports help to make the software more stable and usable.
Please read the pinned [issue #1175](https://github.com/olive-editor/olive/issues/1175)
for guidelines before you create a new issue.

## Writing code

Code contributions are welcome. Note that the code base is rapidly changing in
the current stage of development however. There is some documentation in the form
of code comments, including Javadoc in header files. Feel free to reach out via
an issue or pull request if you have questions about the architecture or
implementation details.

### Code Standards

In order to keep the code as readable and maintainable as possible, code
submitted should abide by the following standards:

* The code style generally follows the
  [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
  including, but not limited to:
  * Indentation is 2 spaces wide, spaces only (no tabs)
  * `lowercase_underscored_variable_names`
  * `lowercase_underscored_functions()` or `SentenceCaseFunctions()`
  * `class SentenceCaseClassesAndStructs {}`
  * `kSentenceCaseConstants` prepended with a lowercase `k`
  * `UPPERCASE_UNDERSCORED_MACROS` for variables or same style as functions for macro functions
  * `class_member_variables_` end with a `_`
* 100 column limit (where it doesn't impair readability)
* Unix line endings (only LF no CRLF)
* Javadoc documentation where appropriate
