# How to create an Olive style

Olive supports customization of its interface through CSS (Qt CSS) and icon replacements.

To create a style, you'll need to create a CSS file called `style.css` and SVG icons. Feel free to duplicate an existing
theme for reference.

To save performance, Olive doesn't actually use the SVGs directly and will need them to be converted to multiple-size
PNGs. You don't need to worry about this step though, `generate-style.sh` will do this for you automatically. You'll
need to re-run this script any time you change an SVG to update the PNG.

For internal use, `generate-style.sh` will also create a QRC so the style can be compiled into Olive. You'll need to
manually add the QRC to `ui/style/CMakeLists.txt` though.