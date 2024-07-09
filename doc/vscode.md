Integration with vscode
-----------------------
It's easy to produce output for parsing with the [coverage gutters](https://marketplace.visualstudio.com/items?itemName=ryanluker.vscode-coverage-gutters)
extension to vscode. The `--cobertura-only` option is well-suited for this, which only outputs a
`cov.xml` file and the coverage database in the output directory. Coverage information can then
be directly shown in the "gutter" beside the editor, and is automatically updated by the
extension when the coverage changes.

To produce this output, just run

```
kcov --cobertura-only [other options] /path/to/your/folder/.vscode /path/to/the/binary
```

The coverage gutters extension looks for `cov.xml` files in the workspace, but for performance
reasons, it's good to configure it to only look where the kcov output is (`.vscode`) in this
example.
