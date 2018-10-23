# Project Syntax

```c
// predefined target
native_lib("xx") {
    debug.win32 = [""]
    release.win32 = ""
    runtime_lib.win32 = ""
    defines = [""]
    include_dir = ""
}

// c family project, c,cxx,objc,objc++
cproj("nano") {
    type = "program|static_lib|shared_lib"
    srcs = []
    cxxflag = ""
    cflags = ""
    objcflags = ""
    objcxxflags = ""
    private.defines = [""] // private scope
    private.inc_dirs = [""]
    match(target_os) { # match expression
        "mac" {

        }
        "windows" {

        }
        "android" {

        }
        "ios" {

        }
    }
    depends = "ddd"
    link_targets = ["xx"]

    on_init = () { // lambda delayed function
        download_and_extract(predownload_url, "../temp")
        exec("xbgn --build ../")
    }
    on_prebuild = () {
    }
    on_post_build = () {
        pack(target)
    }
}

// project inherit
cproj("nano_test" : "nano") {
    // inherit public variables without private scope
    type = "program" // override variables
}

// csharp project
csproj("csharp") {
    sources = []
}

// following is project of rustlang
rustproj("rust") {
    
}

```