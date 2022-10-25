del html\*.* xml\*.* html_sphinx\*.* /Q
doxygen nrfx_examples.doxyfile
sphinx-build -b html sphinx html_sphinx
