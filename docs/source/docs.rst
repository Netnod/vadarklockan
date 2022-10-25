Generating Documentation
========================

The documentation for "vad är klockan" is generated using Sphinx.  The
Python API documentation are generated from docstrings in the code.
The C API documenation is generated from Doxygen comments in the code
and pulled into Sphinx using Breathe.  Some of the documentation is
written i Jupyter Notebook which pulled into sphinx using nbsphinx.

Prerequisites
-------------

To be able to build the documentation, install some required system
packages and Python packages.  On a Debian/Ubuntu system run:

.. highlight:: bash

::

    sudo apt-get -y install doxygen pandoc
    pip3 install -r requirements.txt

HTML output
-----------

Enter the "docs" directory and run the following command do build the
documentation:

.. highlight:: bash

::

    make clean html

Open the file "docs/build/html/index.html" in a browser to read the
generated documentation.

EPUB output
-----------

It's also possible to generate documentation in other formats, for
example, to produce an EPUB suitable for e-readers run:

.. highlight:: bash

::

    make clean epub

The generated file will be called "build/epub/Vadrklockan.epub".

Checking links
--------------

The documentation contain links to other pages.  To check that all
links are working, run:

.. highlight:: bash

::

    make linkcheck

.. _doctest:

Testcases
---------

If there are any code examples in the Python docstrings they can be
used as test cases.  To use those test cases, run:

.. highlight:: bash

::

    make doctest

For example, the following docstring:

.. highlight:: python

::

    """
    >>> print("Hello World!")
    Hello World
    """

will produce and error because there the actual output does not match the
expected output, an exclamation mark is missing:

.. highlight:: none

::

    Failed example:
        print("Hello World!")
    Expected:
        Hello World
    Got:
        Hello World!

Fix the test by adding the missing exclamation mark:

.. highlight:: python

::

    """
    >>> print("Hello World!")
    Hello World!
    """

"make doctest" should now succeed.
