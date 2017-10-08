Travis-ci / codecov integration
---------------------------------
Integrating with [codecov](http://codecov.io) is easy to do. To upload data from the travis build to codecov, run kcov normally, and then upload using the codecov uploader

```
kcov /path/to/outdir executable
bash <(curl -s https://codecov.io/bash) -s /path/to/outdir
```

The easiest way to achieve this is to run the codecov uploader on travis success:

```
after_success:
  - bash <(curl -s https://codecov.io/bash) -s /path/to/outdir
```

the -s /path/to/outdir part can be skipped if kcov produces output in the current directory.
