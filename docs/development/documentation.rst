********************
Documentation System
********************

This documentation is written in *reStructuredText* (reST) syntax and uses
`Sphinx`_ as tool to generate different output formats such as HTML and PDF.

The code documentation is extracted by `Doxygen`_ from JavaDoc-style comments
in the source code (in ``.h`` header files). `Breathe`_ is used as bridge to
Sphinx: its ``breathe-apidoc`` tool generates ``.rst`` files from Doxygen XML.

.. _Sphinx: https://www.sphinx-doc.org/
.. _Doxygen: https://doxygen.nl/
.. _Breathe: https://github.com/michaeljones/breathe

reStructuredText syntax
=======================

reST is the default plaintext markup language used by Sphinx. There are some
differences to the vanilla `Docutils reST`_ syntax, mainly extensions to
support additional functionality.

.. seealso::

   A full syntax description is available in the `Sphinx reST documentation`_

.. _Docutils reST: https://docutils.sourceforge.io/rst.html
.. _Sphinx reST documentation: https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html

Headings
--------

Heading levels are not determined by the characters used, but from
the succession of headings, using different characters.

::

   ####################
   Part (with overline)
   ####################

   ************************
   Chapters (with overline)
   ************************

   Section
   =======

   Subsection
   ----------

   Subsubsection
   ^^^^^^^^^^^^^

   Paragraph
   """""""""

Emphasis
--------

::

   *This text will be italic*
   **This text will be bold**
   *This text will be italic with a **bold** word*

Lists
^^^^^

::

   * Bulleted lists
   * with two items, the second
     using two lines (note the indentation)
      * Sub list
      * with two items

   1. Explicitly numbered list
   2. with two items

   #. Implicitly numbered list
   #. with two items

Code
^^^^

::

   ``inline code``

   ::

      literal code block
      with multiple lines

      until text is less indented

   This is normal text again

Links
^^^^^

Internal::

   .. _my_label:

   Content goes here

   Link to the label (also across files): :ref:`my_label`

External::

   http://www.example.org

   `http://www.example.org`_

   `Link label <http://www.example.org>`_

   `Link label`_

   .. _Link label: http://www.example.org

Images
^^^^^^

Just an image without caption::

   .. image:: filename.ext
      :width: 200px
      :height: 100px
      :align: center
      :alt: alternate text

A figure (image with caption)::

   .. figure:: filename.ext
      :width: 200px
      :height: 100px
      :align: center
      :alt: alternate text
      :figclass: some-css-class

      Caption text goes here

      Additional content (?)

Boxes
^^^^^

Neutral callout::

   .. note:: This is a note box.
      Note the space between ``::`` and ``This``

Warning callout::

   .. warning:: This is a warning box!

Comments
^^^^^^^^

::

   .. Single line comment

   This is normal text.

   ..
      This is a comment block
      with multiple lines

      until text is less indented

   This is normal text again.

Style and Spelling
------------------

Lines should generally not exceed a maximum of 80 characters in source.
This helps both terminal users and version control.

American English?

Generate the Documentation
--------------------------

