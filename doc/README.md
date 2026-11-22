# Building the documentation

From the repository root, use Python 3.10 or later in a virtual environment.

```sh
python -m pip install -r doc/requirements.txt
python -m sphinx -W -n -b html -d build/doctrees doc build/html
python -m http.server --bind 127.0.0.1 --directory build/html 8000
```

Open <http://localhost:8000>. Edit the getting-started guide and worked example
in `doc/`. The reference pages include `FORMAT.md` and `BINDINGS.md` from the
repository root.

## GitHub Pages

1. Open **Settings → Pages → Build and deployment** and set **Source** to **GitHub Actions**.
2. Open **Actions → Documentation → Run workflow**, select `master`, and run it.

For a private repository, the GitHub plan must include Pages for private repositories.

Changes to the documentation and examples on `master` publish automatically.
The deployment job reports the site URL. The default address for this repository
is <https://cmower.github.io/ConFiGifY/>.
