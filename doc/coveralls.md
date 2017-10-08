Travis-ci / coveralls integration
---------------------------------
kcov is easy to integrate with [travis-ci](http://travis-ci.org) and
[coveralls.io](http://coveralls.io). To upload data from the travis build to coveralls,
run kcov with

```
kcov --coveralls-id=$TRAVIS_JOB_ID /path/to/outdir executable
```

which in addition to regular coverage collection uploads to coveralls.
