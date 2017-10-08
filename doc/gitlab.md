GitLab CI integration
---------------------
Kcov generates a very generic json file which includes the overall percent covered
for a single command and the count of lines instrumented and covered. It also includes
a summary of each source file with a percentage and line counts. This allows easy
integration with GitlabCI (see https://docs.gitlab.com/ce/user/project/pipelines/settings.html).

The JSON output is placed in a file named ```out-path/exec-filename/coverage.json```.
