# Building the documentation

## Doxygen

Generate the Doxygen-based documentation by running:

```shell
doxygen nrfx_examples.doxyfile
```

You can also use the provided script `generate_html_doc.sh` located in the `doc` folder.
The output can be viewed by navigating to `html` folder and opening `index.html` file.

## Sphinx

All the necessary files to compile the Sphinx based documentation for `nrfx_examples`
are located under `sphinx` folder. As of today the content should match with
the one produced using `doxygen` only.

### Requirements

You will need to have Python 3 installed as well as some dependencies, which can
be installed by running:

```shell
pip install -r requirements.txt
```

### Build

You may want to use the provided script `generate_sphinx_doc.sh`. The result can be viewed by opening `html_sphinx/index.html`.

If you want to do it manually you can run the following commands from `doc`
directory:

```shell
# compile doxygen documentation (required to generate XML metadata)
doxygen nrfx_examples.doxyfile
# compile Sphinx documentation
sphinx-build -b html sphinx html_sphinx
```
