Travis-ci / coveralls integration
---------------------------------
kcov is easy to integrate with [travis-ci](https://www.travis-ci.com/) and
[coveralls.io](https://coveralls.io). To upload data from the travis build to coveralls,
run kcov with

```sh
kcov --coveralls-id=$TRAVIS_JOB_ID /path/to/outdir executable
```

which in addition to regular coverage collection uploads to coveralls.
