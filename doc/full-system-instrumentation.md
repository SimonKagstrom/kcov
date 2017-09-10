## *Full system instrumentation with kcov*
Prerequisites
-------------
You need to install patchelf >= 0.9 on your system before recording. Patchelf can be found
on https://github.com/NixOS/patchelf, or as a package on some distributions (note the version
though!).


Add the kcov-system-daemon to your target system
------------------------------------------------
You need to add kcov-system-daemon to your target system and start it early during system
bootup. This is needed in order to process instrumentation points and write report data.

Instrumenting binaries for a target system
------------------------------------------
If your binaries (with debug symbols) are in e.g., sysroot, you can instrument binaries using

```
kcov-system --system-record /tmp/out-sysroot sysroot
```

After this finishes, build your filesystem (squashfs etc) using the instrumented binaries
and install and run on your target.

Note that ```--system-record``` also produces a shared library (libkcov_system_lib) which
needs to be available in the library on the target system. kcov places it in ```/lib``` under
```/tmp/out-sysroot```.

Creating a report from the collected data
-----------------------------------------
Copy `/tmp/kcov-data` from your target system and run

```
kcov-system --system-report /tmp/kcov /tmp/kcov-data
```
