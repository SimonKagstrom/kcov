Integration with Jenkins
------------------------
Kcov also outputs data in the Cobertura XML format, which allows integrating kcov
output in Jenkins (see https://cobertura.github.io/cobertura/ and https://www.jenkins.io/).

The Cobertura output is placed in a file named ```out-path/exec-filename/cobertura.xml```
