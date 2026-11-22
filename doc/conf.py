project = "ConFiGifY"
author = "Christopher E. Mower"

extensions = ["myst_parser"]
exclude_patterns = ["README.md"]
myst_heading_anchors = 3

html_theme = "sphinx_rtd_theme"
templates_path = ["_templates"]
html_logo = "_static/configify.svg"
html_title = "ConFiGifY documentation"
html_short_title = "ConFiGifY"
html_theme_options = {"navigation_depth": 2, "style_nav_header_background": "#233b50"}
html_static_path = ["_static"]
html_css_files = ["manual.css"]
html_copy_source = False
html_show_sourcelink = False
html_show_sphinx = False
html_use_index = False
