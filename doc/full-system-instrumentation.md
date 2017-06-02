## *Full system instrumentation with kcov*

Instrumenting binaries for a target system
------------------------------------------
If your binaries (with debug symbols) are in e.g., sysroot, you can instrument binaries using

```
kcov-system --system-record /tmp/out-sysroot sysroot
```

After this finishes, build your filesystem (squashfs etc) using the instrumented binaries
and install and run on your target.


Creating a report from the collected data
-----------------------------------------
Copy `/tmp/kcov-data` from your target system and run

```
kcov-system --system-report /tmp/kcov /tmp/kcov-data
```
