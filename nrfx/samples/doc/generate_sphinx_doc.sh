rm -rf html xml html_sphinx
doxygen nrfx_examples.doxyfile
sphinx-build -b html sphinx html_sphinx
