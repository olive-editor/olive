# Notes

## Sphinx Setup

- Assuming Python 3 (check with `python --version`),
  e.g. `choco install python3`

- Updating pip doesn't hurt

  ```
  python -m pip install --upgrade pip
  ```

- Install `wheel` package to avoid warnings.

  (`<Package>` = setuptools, six, pytz)

  ```
  Could not build wheels for <Package>, since package 'wheel' is not installed.
  ```

  ```
  python -m pip install wheel
  ```

- Ensure that Python's `Scripts` folder is in `PATH` environment variable to
  be able to run tools later.

  (`<Executable>` = chardetect.exe, pygmentize.exe, pybabel.exe, sphinx-apidoc.exe,
  sphinx-autogen.exe, sphinx-build.exe, sphinx-quickstart.exe)

  ```
  WARNING: The script <Executable> is installed in 'C:\Users\<User>\AppData\Local\Programs\Python\Python37-32\Scripts' which is not on PATH.
  ```

- Install Sphinx

  ```
  python -m pip install sphinx
  ```

- Configure Sphinx

  ```
  sphinx-quickstart
  ```

- Install breathe (Sphinx bridge for doxygen)

  ```
  python -m pip install breathe
  ```

## Theme Setup

To use RTD Neo theme, adjust `conf.py`:

```py
html_theme = 'neo_rtd_theme'
import sphinx_theme
html_theme_path = [sphinx_theme.get_html_theme_path()]
```

Install the theme:

```
python -m pip install sphinx-theme
```

## Dependency Management

Automated installation of extensions/themes via pip's `requirements.txt`:

```
Sphinx ~= 3.0.3
sphinx_rtd_theme ~= 0.4.3
breathe ~= 4.18.0
sphinx-tabs ~= 2.1.0
```

https://pip.pypa.io/en/stable/reference/pip_install/#example-requirements-file

Install:

```
pip install -r requirements.txt
```

Upgrade:

```
pip install -r requirements.txt --upgrade
```

## Building

- Build HTML documentation

  ```
  make html
  ```

  which is a shorthand for

  ```
  sphinx-build . _build/html -b html
  ```

  TODO: How to use HTML5 translator? (seems to be just HTML currently).
  `-b html5` does not work.

- Serve HTML at http://localhost:8000/

  ```
  python -m http.server --directory _build\html
  ```

- Generate code documentation

  ```
  cd docs
  doxygen
  make html
  ```

  Generate file, class and other lists in .rst format with `breathe-apidoc`:
  https://github.com/michaeljones/breathe/blob/master/breathe/apidoc.py

  ```
  breathe-apidoc _doxygen/xml -o apidoc
  ```

  Alternative: https://github.com/svenevs/exhale
